$ErrorActionPreference = 'Stop'

if (-not ('CameraTweaks.Testing.ProcessTreeSupervisor' -as [type])) {
    $source = @'
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using Microsoft.Win32.SafeHandles;

namespace CameraTweaks.Testing {
    public sealed class ProcessTreeResult {
        public int ExitCode { get; internal set; }
        public bool TimedOut { get; internal set; }
        public bool DescendantsOutlivedRoot { get; internal set; }
        public bool ExpectedDescendantsCleaned { get; internal set; }
        public bool CleanupVerified { get; internal set; }
        public bool OutputTruncated { get; internal set; }
        public string RemainingProcesses { get; internal set; }
        public string Output { get; internal set; }
        public uint ProcessId { get; internal set; }
    }

    public static class ProcessTreeSupervisor {
        private sealed class OwnedProcessHandle {
            public uint ProcessId;
            public IntPtr Handle;
        }
        private const uint CreateSuspended = 0x00000004;
        private const uint CreateNoWindow = 0x08000000;
        private const uint StartfUseStdHandles = 0x00000100;
        private const uint HandleFlagInherit = 0x00000001;
        private const uint JobObjectLimitKillOnJobClose = 0x00002000;
        private const uint Synchronize = 0x00100000;
        private const uint ProcessQueryLimitedInformation = 0x1000;
        private const uint WaitObject0 = 0;
        private const uint WaitTimeout = 258;
        private const uint StdInputHandle = unchecked((uint)-10);
        private const uint ExitTimeout = 124;
        private const int RootExitDescendantGraceMilliseconds = 2000;

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
        private struct StartupInfo {
            public int cb;
            public string lpReserved;
            public string lpDesktop;
            public string lpTitle;
            public uint dwX, dwY, dwXSize, dwYSize, dwXCountChars, dwYCountChars;
            public uint dwFillAttribute, dwFlags;
            public short wShowWindow, cbReserved2;
            public IntPtr lpReserved2, hStdInput, hStdOutput, hStdError;
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct ProcessInformation {
            public IntPtr hProcess, hThread;
            public uint dwProcessId, dwThreadId;
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct SecurityAttributes {
            public int nLength;
            public IntPtr lpSecurityDescriptor;
            [MarshalAs(UnmanagedType.Bool)] public bool bInheritHandle;
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct BasicLimitInformation {
            public long PerProcessUserTimeLimit, PerJobUserTimeLimit;
            public uint LimitFlags;
            public UIntPtr MinimumWorkingSetSize, MaximumWorkingSetSize;
            public uint ActiveProcessLimit;
            public UIntPtr Affinity;
            public uint PriorityClass, SchedulingClass;
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct IoCounters {
            public ulong ReadOperationCount, WriteOperationCount, OtherOperationCount;
            public ulong ReadTransferCount, WriteTransferCount, OtherTransferCount;
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct ExtendedLimitInformation {
            public BasicLimitInformation BasicLimitInformation;
            public IoCounters IoInfo;
            public UIntPtr ProcessMemoryLimit, JobMemoryLimit;
            public UIntPtr PeakProcessMemoryUsed, PeakJobMemoryUsed;
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct BasicAccountingInformation {
            public long TotalUserTime, TotalKernelTime;
            public long ThisPeriodTotalUserTime, ThisPeriodTotalKernelTime;
            public uint TotalPageFaultCount, TotalProcesses, ActiveProcesses, TotalTerminatedProcesses;
        }

        [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
        private static extern IntPtr CreateJobObject(IntPtr attributes, string name);
        [DllImport("kernel32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        private static extern bool SetInformationJobObject(IntPtr job, int informationClass,
            IntPtr information, uint informationLength);
        [DllImport("kernel32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        private static extern bool QueryInformationJobObject(IntPtr job, int informationClass,
            IntPtr information, uint informationLength, out uint returnLength);
        [DllImport("kernel32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        private static extern bool AssignProcessToJobObject(IntPtr job, IntPtr process);
        [DllImport("kernel32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        private static extern bool TerminateJobObject(IntPtr job, uint exitCode);
        [DllImport("kernel32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        private static extern bool CreatePipe(out IntPtr readPipe, out IntPtr writePipe,
            ref SecurityAttributes attributes, uint size);
        [DllImport("kernel32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        private static extern bool SetHandleInformation(IntPtr handle, uint mask, uint flags);
        [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
        [return: MarshalAs(UnmanagedType.Bool)]
        private static extern bool CreateProcess(string applicationName, StringBuilder commandLine,
            IntPtr processAttributes, IntPtr threadAttributes, bool inheritHandles, uint creationFlags,
            IntPtr environment, string currentDirectory, ref StartupInfo startupInfo,
            out ProcessInformation processInformation);
        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern uint ResumeThread(IntPtr thread);
        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern uint WaitForSingleObject(IntPtr handle, uint milliseconds);
        [DllImport("kernel32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        private static extern bool GetExitCodeProcess(IntPtr process, out uint exitCode);
        [DllImport("kernel32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        private static extern bool TerminateProcess(IntPtr process, uint exitCode);
        [DllImport("kernel32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        private static extern bool CloseHandle(IntPtr handle);
        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern IntPtr GetStdHandle(uint standardHandle);
        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern IntPtr OpenProcess(uint access, bool inheritHandle, uint processId);
        [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
        [return: MarshalAs(UnmanagedType.Bool)]
        private static extern bool QueryFullProcessImageName(IntPtr process, uint flags,
            StringBuilder imageName, ref uint size);

        public static ProcessTreeResult Run(string executable, string[] arguments, string workingDirectory,
            int timeoutMilliseconds, int cleanupTimeoutMilliseconds, bool noWindow, int maximumOutputCharacters,
            bool streamOutput, string[] expectedResidualProcessNames) {
            var commandArguments = new StringBuilder();
            if (arguments != null) foreach (string argument in arguments) {
                if (commandArguments.Length != 0) commandArguments.Append(' ');
                commandArguments.Append(Quote(argument ?? String.Empty));
            }
            return RunRaw(executable, commandArguments.ToString(), workingDirectory, timeoutMilliseconds,
                cleanupTimeoutMilliseconds, noWindow, maximumOutputCharacters, streamOutput,
                expectedResidualProcessNames);
        }

        public static ProcessTreeResult RunRaw(string executable, string rawArguments, string workingDirectory,
            int timeoutMilliseconds, int cleanupTimeoutMilliseconds, bool noWindow, int maximumOutputCharacters,
            bool streamOutput, string[] expectedResidualProcessNames) {
            if (String.IsNullOrWhiteSpace(executable) || timeoutMilliseconds <= 0 ||
                cleanupTimeoutMilliseconds <= 0 || maximumOutputCharacters <= 0)
                throw new ArgumentOutOfRangeException("A finite positive process, cleanup, and output budget is required.");

            IntPtr job = IntPtr.Zero, pipeRead = IntPtr.Zero, pipeWrite = IntPtr.Zero;
            ProcessInformation process = new ProcessInformation();
            Task<Tuple<string, bool>> outputTask = null;
            FileStream outputStream = null;
            bool timedOut = false, cleanupVerified = false, descendantsOutlivedRoot = false;
            bool expectedDescendantsCleaned = false;
            string remainingProcesses = String.Empty;
            string output = String.Empty;
            bool outputTruncated = false;
            var clock = System.Diagnostics.Stopwatch.StartNew();
            try {
                job = CreateJobObject(IntPtr.Zero, null);
                if (job == IntPtr.Zero) throw LastError("CreateJobObject");
                var limits = new ExtendedLimitInformation();
                limits.BasicLimitInformation.LimitFlags = JobObjectLimitKillOnJobClose;
                IntPtr limitBuffer = Marshal.AllocHGlobal(Marshal.SizeOf(typeof(ExtendedLimitInformation)));
                try {
                    Marshal.StructureToPtr(limits, limitBuffer, false);
                    if (!SetInformationJobObject(job, 9, limitBuffer,
                        (uint)Marshal.SizeOf(typeof(ExtendedLimitInformation)))) throw LastError("SetInformationJobObject");
                } finally { Marshal.FreeHGlobal(limitBuffer); }

                var security = new SecurityAttributes();
                security.nLength = Marshal.SizeOf(typeof(SecurityAttributes));
                security.bInheritHandle = true;
                if (!CreatePipe(out pipeRead, out pipeWrite, ref security, 0)) throw LastError("CreatePipe");
                if (!SetHandleInformation(pipeRead, HandleFlagInherit, 0)) throw LastError("SetHandleInformation");

                var startup = new StartupInfo();
                startup.cb = Marshal.SizeOf(typeof(StartupInfo));
                startup.dwFlags = StartfUseStdHandles;
                startup.hStdInput = GetStdHandle(StdInputHandle);
                startup.hStdOutput = pipeWrite;
                startup.hStdError = pipeWrite;
                var command = new StringBuilder(Quote(executable));
                if (!String.IsNullOrWhiteSpace(rawArguments)) command.Append(' ').Append(rawArguments);
                uint flags = CreateSuspended | (noWindow ? CreateNoWindow : 0);
                if (!CreateProcess(executable, command, IntPtr.Zero, IntPtr.Zero, true, flags,
                    IntPtr.Zero, workingDirectory, ref startup, out process)) throw LastError("CreateProcess");

                CloseHandle(pipeWrite); pipeWrite = IntPtr.Zero;
                if (!AssignProcessToJobObject(job, process.hProcess)) {
                    int assignError = Marshal.GetLastWin32Error();
                    TerminateProcess(process.hProcess, 125);
                    uint stopped = WaitForSingleObject(process.hProcess, (uint)cleanupTimeoutMilliseconds);
                    if (stopped != WaitObject0) {
                        TerminateProcess(process.hProcess, 125);
                        stopped = WaitForSingleObject(process.hProcess, (uint)cleanupTimeoutMilliseconds);
                    }
                    if (stopped != WaitObject0) {
                        throw new InvalidOperationException("AssignProcessToJobObject failed and termination of the suspended child could not be verified; PID=" + process.dwProcessId + ", wait=" + stopped + ".");
                    }
                    throw new Win32Exception(assignError, "AssignProcessToJobObject failed; suspended child termination was verified.");
                }

                outputStream = new FileStream(new SafeFileHandle(pipeRead, true), FileAccess.Read, 4096, false);
                pipeRead = IntPtr.Zero;
                outputTask = Task.Run(() => ReadBounded(outputStream, maximumOutputCharacters, streamOutput));
                if (ResumeThread(process.hThread) == UInt32.MaxValue) throw LastError("ResumeThread");
                CloseHandle(process.hThread); process.hThread = IntPtr.Zero;

                bool rootExited = false;
                long rootExitObservedAt = -1;
                while (true) {
                    rootExited = WaitForSingleObject(process.hProcess, 0) == WaitObject0;
                    uint active = ActiveProcessCount(job);
                    if (rootExited && active == 0) { cleanupVerified = true; break; }
                    if (rootExited && active != 0) {
                        if (rootExitObservedAt < 0) rootExitObservedAt = clock.ElapsedMilliseconds;
                        if (clock.ElapsedMilliseconds - rootExitObservedAt >= RootExitDescendantGraceMilliseconds) {
                            descendantsOutlivedRoot = true;
                            remainingProcesses = DescribeJobProcesses(job);
                            if (AllJobProcessesExpected(job, expectedResidualProcessNames)) {
                                TerminateJobObject(job, ExitTimeout);
                                cleanupVerified = WaitForTreeEmpty(job, cleanupTimeoutMilliseconds);
                                if (!cleanupVerified) {
                                    string remainingPids;
                                    cleanupVerified = CloseJobAndVerify(job, cleanupTimeoutMilliseconds, out remainingPids);
                                    job = IntPtr.Zero;
                                    if (!cleanupVerified)
                                        throw new InvalidOperationException("Expected build-helper cleanup failed; remaining PIDs: " + remainingPids);
                                }
                                expectedDescendantsCleaned = true;
                                break;
                            }
                            timedOut = true;
                            break;
                        }
                    }
                    if (clock.ElapsedMilliseconds >= timeoutMilliseconds) { timedOut = true; break; }
                    Thread.Sleep(25);
                }

                if (timedOut) {
                    TerminateJobObject(job, ExitTimeout);
                    cleanupVerified = WaitForTreeEmpty(job, cleanupTimeoutMilliseconds);
                    if (!cleanupVerified) {
                        string remainingPids;
                        cleanupVerified = CloseJobAndVerify(job, cleanupTimeoutMilliseconds, out remainingPids);
                        job = IntPtr.Zero;
                        if (!cleanupVerified) throw new InvalidOperationException("Owned test process tree did not exit; remaining PIDs: " + remainingPids);
                    }
                }

                if (!outputTask.Wait(cleanupTimeoutMilliseconds))
                    throw new TimeoutException("Owned test output pipe did not close after the process tree exited.");
                var captured = outputTask.Result;
                output = captured.Item1;
                outputTruncated = captured.Item2;
                uint nativeExitCode;
                if (timedOut) nativeExitCode = ExitTimeout;
                else if (!GetExitCodeProcess(process.hProcess, out nativeExitCode)) throw LastError("GetExitCodeProcess");
                return new ProcessTreeResult { ExitCode = unchecked((int)nativeExitCode), TimedOut = timedOut,
                    DescendantsOutlivedRoot = descendantsOutlivedRoot,
                    ExpectedDescendantsCleaned = expectedDescendantsCleaned,
                    CleanupVerified = cleanupVerified, Output = output, OutputTruncated = outputTruncated,
                    RemainingProcesses = remainingProcesses,
                    ProcessId = process.dwProcessId };
            } catch {
                if (job != IntPtr.Zero) {
                    TerminateJobObject(job, 125);
                    if (!WaitForTreeEmpty(job, cleanupTimeoutMilliseconds)) {
                        string remainingPids;
                        bool verified = CloseJobAndVerify(job, cleanupTimeoutMilliseconds, out remainingPids);
                        job = IntPtr.Zero;
                        if (!verified) throw new InvalidOperationException("Failure cleanup could not verify exit of Job Object processes; remaining PIDs: " + remainingPids);
                    }
                }
                throw;
            } finally {
                if (process.hThread != IntPtr.Zero) CloseHandle(process.hThread);
                if (process.hProcess != IntPtr.Zero) CloseHandle(process.hProcess);
                if (pipeWrite != IntPtr.Zero) CloseHandle(pipeWrite);
                if (pipeRead != IntPtr.Zero) CloseHandle(pipeRead);
                if (outputStream != null) outputStream.Dispose();
                if (job != IntPtr.Zero) CloseHandle(job);
            }
        }

        public static bool IsProcessRunning(uint processId) {
            IntPtr process = OpenProcess(Synchronize | ProcessQueryLimitedInformation, false, processId);
            if (process == IntPtr.Zero) {
                int error = Marshal.GetLastWin32Error();
                if (error == 87 || error == 1168) return false;
                throw new Win32Exception(error, "OpenProcess for cleanup verification failed.");
            }
            try {
                uint state = WaitForSingleObject(process, 0);
                if (state == WaitTimeout) return true;
                if (state == WaitObject0) return false;
                throw LastError("WaitForSingleObject process identity check");
            }
            finally { CloseHandle(process); }
        }

        private static Tuple<string, bool> ReadBounded(Stream stream, int maximumCharacters, bool streamOutput) {
            var text = new StringBuilder(Math.Min(maximumCharacters, 65536));
            var buffer = new char[4096];
            bool truncated = false;
            int read;
            using (var reader = new StreamReader(stream, new UTF8Encoding(false, false), true, 4096, true))
            while ((read = reader.Read(buffer, 0, buffer.Length)) > 0) {
                int remaining = maximumCharacters - text.Length;
                int kept = Math.Min(Math.Max(remaining, 0), read);
                if (streamOutput && kept > 0) { Console.Out.Write(buffer, 0, kept); Console.Out.Flush(); }
                if (remaining > 0) text.Append(buffer, 0, Math.Min(remaining, read));
                if (read > remaining) truncated = true;
            }
            return Tuple.Create(text.ToString(), truncated);
        }

        private static uint ActiveProcessCount(IntPtr job) {
            var accounting = new BasicAccountingInformation();
            IntPtr buffer = Marshal.AllocHGlobal(Marshal.SizeOf(typeof(BasicAccountingInformation)));
            try {
                uint returnedLength;
                if (!QueryInformationJobObject(job, 1, buffer,
                    (uint)Marshal.SizeOf(typeof(BasicAccountingInformation)), out returnedLength)) throw LastError("QueryInformationJobObject");
                accounting = (BasicAccountingInformation)Marshal.PtrToStructure(buffer, typeof(BasicAccountingInformation));
                return accounting.ActiveProcesses;
            } finally { Marshal.FreeHGlobal(buffer); }
        }

        private static bool WaitForTreeEmpty(IntPtr job, int timeoutMilliseconds) {
            var watch = System.Diagnostics.Stopwatch.StartNew();
            while (watch.ElapsedMilliseconds < timeoutMilliseconds) {
                if (ActiveProcessCount(job) == 0) return true;
                Thread.Sleep(25);
            }
            return ActiveProcessCount(job) == 0;
        }

        private static string DescribeJobProcesses(IntPtr job) {
            var handles = OpenJobProcessHandles(job);
            try {
                var descriptions = new List<string>();
                foreach (OwnedProcessHandle process in handles) {
                    if (process.Handle == IntPtr.Zero) continue;
                    var image = new StringBuilder(1024);
                    uint size = (uint)image.Capacity;
                    if (QueryFullProcessImageName(process.Handle, 0, image, ref size))
                        descriptions.Add(process.ProcessId + ":" + image.ToString());
                    else
                        descriptions.Add(process.ProcessId + ":<image-unavailable>");
                }
                return String.Join(";", descriptions.ToArray());
            } finally {
                foreach (OwnedProcessHandle process in handles)
                    if (process.Handle != IntPtr.Zero) CloseHandle(process.Handle);
            }
        }

        private static bool AllJobProcessesExpected(IntPtr job, string[] expectedNames) {
            if (expectedNames == null || expectedNames.Length == 0) return false;
            var expected = new HashSet<string>(expectedNames, StringComparer.OrdinalIgnoreCase);
            var handles = OpenJobProcessHandles(job);
            try {
                if (handles.Count == 0) return false;
                foreach (OwnedProcessHandle process in handles) {
                    if (process.Handle == IntPtr.Zero) return false;
                    var image = new StringBuilder(1024);
                    uint size = (uint)image.Capacity;
                    if (!QueryFullProcessImageName(process.Handle, 0, image, ref size) ||
                        !expected.Contains(Path.GetFileName(image.ToString()))) return false;
                }
                return true;
            } finally {
                foreach (OwnedProcessHandle process in handles)
                    if (process.Handle != IntPtr.Zero) CloseHandle(process.Handle);
            }
        }

        private static bool CloseJobAndVerify(IntPtr job, int timeoutMilliseconds, out string remainingPids) {
            var handles = OpenJobProcessHandles(job);
            CloseHandle(job);
            bool allExited = true;
            var watch = System.Diagnostics.Stopwatch.StartNew();
            try {
                while (watch.ElapsedMilliseconds < timeoutMilliseconds) {
                    bool pending = false;
                    foreach (OwnedProcessHandle process in handles) {
                        if (process.Handle == IntPtr.Zero) continue;
                        uint state = WaitForSingleObject(process.Handle, 0);
                        if (state == WaitTimeout) pending = true;
                        else if (state != WaitObject0) allExited = false;
                    }
                    if (!pending) {
                        remainingPids = allExited ? String.Empty : String.Join(",", handles.ConvertAll(item => item.ProcessId.ToString()).ToArray());
                        return allExited;
                    }
                    Thread.Sleep(25);
                }
                var stillRunning = new List<string>();
                foreach (OwnedProcessHandle process in handles)
                    if (process.Handle != IntPtr.Zero && WaitForSingleObject(process.Handle, 0) != WaitObject0) {
                        allExited = false;
                        stillRunning.Add(process.ProcessId.ToString());
                    }
                remainingPids = String.Join(",", stillRunning.ToArray());
                return allExited;
            } finally { foreach (OwnedProcessHandle process in handles) if (process.Handle != IntPtr.Zero) CloseHandle(process.Handle); }
        }

        private static List<OwnedProcessHandle> OpenJobProcessHandles(IntPtr job) {
            int capacity = 64;
            while (capacity <= 65536) {
                int size = 8 + capacity * IntPtr.Size;
                IntPtr buffer = Marshal.AllocHGlobal(size);
                try {
                    uint returned;
                    if (!QueryInformationJobObject(job, 3, buffer, (uint)size, out returned)) throw LastError("QueryInformationJobObject process list");
                    uint listed = unchecked((uint)Marshal.ReadInt32(buffer, 4));
                    if (listed > capacity) { capacity = checked((int)listed); continue; }
                    var handles = new List<OwnedProcessHandle>();
                    for (int index = 0; index < listed; ++index) {
                        ulong raw = IntPtr.Size == 8 ? unchecked((ulong)Marshal.ReadInt64(buffer, 8 + index * IntPtr.Size)) : unchecked((uint)Marshal.ReadInt32(buffer, 8 + index * IntPtr.Size));
                        uint pid = unchecked((uint)raw);
                        IntPtr handle = OpenProcess(Synchronize | ProcessQueryLimitedInformation, false, pid);
                        if (handle != IntPtr.Zero) handles.Add(new OwnedProcessHandle { ProcessId = pid, Handle = handle });
                        else {
                            int error = Marshal.GetLastWin32Error();
                            if (error != 87 && error != 1168) throw new Win32Exception(error, "Could not retain process handle for cleanup verification: " + pid);
                        }
                    }
                    return handles;
                } finally { Marshal.FreeHGlobal(buffer); }
            }
            throw new InvalidOperationException("Test process tree exceeded the bounded PID inventory capacity.");
        }

        private static string Quote(string argument) {
            if (argument.Length != 0 && argument.IndexOfAny(new char[] {' ', '\t', '\n', '\v', '"'}) < 0) return argument;
            var result = new StringBuilder(); result.Append('"');
            int slashes = 0;
            foreach (char value in argument) {
                if (value == '\\') { ++slashes; continue; }
                if (value == '"') { result.Append('\\', slashes * 2 + 1).Append('"'); slashes = 0; continue; }
                result.Append('\\', slashes).Append(value); slashes = 0;
            }
            result.Append('\\', slashes * 2).Append('"');
            return result.ToString();
        }

        private static Exception LastError(string operation) {
            return new Win32Exception(Marshal.GetLastWin32Error(), operation + " failed.");
        }
    }
}
'@
    Add-Type -TypeDefinition $source -Language CSharp -ErrorAction Stop
}

function Invoke-TestProcessTree {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)][string] $FileName,
        [string[]] $ArgumentList = @(),
        [Parameter(Mandatory = $true)][string] $WorkingDirectory,
        [Parameter(Mandatory = $true)][ValidateRange(1, 3600000)][int] $TimeoutMilliseconds,
        [ValidateRange(1, 30000)][int] $CleanupTimeoutMilliseconds = 5000,
        [switch] $NoWindow,
        [switch] $StreamOutput,
        [ValidateRange(1024, 4194304)][int] $MaximumOutputCharacters = 1048576,
        [string[]] $ExpectedResidualProcessNames = @()
    )
    $fullFile = [IO.Path]::GetFullPath($FileName)
    $fullWorkingDirectory = [IO.Path]::GetFullPath($WorkingDirectory)
    if (-not (Test-Path -LiteralPath $fullFile -PathType Leaf)) { throw "Process executable is missing: $fullFile" }
    if (-not (Test-Path -LiteralPath $fullWorkingDirectory -PathType Container)) { throw "Process working directory is missing: $fullWorkingDirectory" }
    $result = [CameraTweaks.Testing.ProcessTreeSupervisor]::Run(
        $fullFile, [string[]]$ArgumentList, $fullWorkingDirectory,
        $TimeoutMilliseconds, $CleanupTimeoutMilliseconds, [bool]$NoWindow,
        $MaximumOutputCharacters, [bool]$StreamOutput, [string[]]$ExpectedResidualProcessNames)
    if (-not $result.CleanupVerified) { throw "Process tree cleanup was not verified for PID $($result.ProcessId)." }
    return $result
}

function Invoke-TestCommandProcess {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)][string] $FileName,
        [Parameter(Mandatory = $true)][string] $RawArguments,
        [Parameter(Mandatory = $true)][string] $WorkingDirectory,
        [Parameter(Mandatory = $true)][ValidateRange(1, 3600000)][int] $TimeoutMilliseconds,
        [ValidateRange(1, 30000)][int] $CleanupTimeoutMilliseconds = 5000,
        [switch] $NoWindow,
        [ValidateRange(1024, 4194304)][int] $MaximumOutputCharacters = 1048576,
        [string[]] $ExpectedResidualProcessNames = @()
    )
    $fullFile = [IO.Path]::GetFullPath($FileName)
    $fullWorkingDirectory = [IO.Path]::GetFullPath($WorkingDirectory)
    if (-not (Test-Path -LiteralPath $fullFile -PathType Leaf)) { throw "Process executable is missing: $fullFile" }
    if (-not (Test-Path -LiteralPath $fullWorkingDirectory -PathType Container)) { throw "Process working directory is missing: $fullWorkingDirectory" }
    $result = [CameraTweaks.Testing.ProcessTreeSupervisor]::RunRaw(
        $fullFile, $RawArguments, $fullWorkingDirectory,
        $TimeoutMilliseconds, $CleanupTimeoutMilliseconds, [bool]$NoWindow,
        $MaximumOutputCharacters, $false, [string[]]$ExpectedResidualProcessNames)
    if (-not $result.CleanupVerified) { throw "Process tree cleanup was not verified for PID $($result.ProcessId)." }
    return $result
}

Export-ModuleMember -Function Invoke-TestProcessTree, Invoke-TestCommandProcess
