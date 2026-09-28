# WinDbg capture: T2 → T3 ResizeBuffers chain formation

## Вихідна точка та підготовлений symbol build

Шляхи підтверджені наданим WinDbg transcript:

- Game executable: `E:\Steam\steamapps\common\S.T.A.L.K.E.R. 2 Heart of Chornobyl\Stalker2\Binaries\Win64\Stalker2-Win64-Shipping.exe`
- Завантажений canonical ASI: `E:\Steam\steamapps\common\S.T.A.L.K.E.R. 2 Heart of Chornobyl\Stalker2\Binaries\Win64\STALKER2CameraTweaks.asi`
- У тому процесі завантажені game-local `Binaries\Win64\dxgi.dll` і system `C:\Windows\System32\dxgi.dll`.

Production build не містить private PDB. Я підготував окремий symbol-enabled build із тим самим production source/profile та output у `build-artifacts\windbg-symbols`; кореневий production ASI не був target-ом збірки. Diagnostic ASI має той самий canonical basename, але **інший SHA-256**, тому PDB не можна під’єднати до вже запущеного процесу зі старим ASI. Для цього capture потрібно після безпечної заміни файлу створити новий процес із symbol-enabled ASI. Код гри й production ASI не змінюються.

Prepared files:

- `build-artifacts\windbg-symbols\STALKER2CameraTweaks.asi`
- `build-artifacts\windbg-symbols\STALKER2CameraTweaks.pdb`

Diagnostic ASI побудовано з `/Zi`, `/DEBUG:FULL`, `/OPT:REF`, `/OPT:ICF`, `/INCREMENTAL:NO`; production feature defines залишені. PE має той самий `SizeOfImage` і розмір `.text` секції, що й production ASI. Бінарні файли не є byte-identical.

## Установлення diagnostic ASI — тільки після повного закриття гри

Поточна WinDbg-сесія завантажила production ASI, тому не продовжуй її для symbols capture. Після виходу з гри виконай цей PowerShell блок. Він перевіряє, що процес завершився, зберігає встановлений production файл поза game directory і ставить diagnostic build під canonical ім’ям. Backup не лежить у game directory, отже не є другою ASI для loader-а.

```powershell
$gameExe = 'E:\Steam\steamapps\common\S.T.A.L.K.E.R. 2 Heart of Chornobyl\Stalker2\Binaries\Win64\Stalker2-Win64-Shipping.exe'
$gameDir = Split-Path -Parent $gameExe
$installedAsi = Join-Path $gameDir 'STALKER2CameraTweaks.asi'
$productionBackup = 'E:\Work\Slaker2 mods\01-Projects\STALKER-2-Ultrawide-Fix-for-UE-5.5.4\build-artifacts\windbg-symbols\STALKER2CameraTweaks-production-backup.asi'
$symbolAsi = 'E:\Work\Slaker2 mods\01-Projects\STALKER-2-Ultrawide-Fix-for-UE-5.5.4\build-artifacts\windbg-symbols\STALKER2CameraTweaks.asi'
$expectedProductionHash = '6C76FF71D530A362B2B14F3FC1BE5EBD9D30D899495206FF885DC7258452347F'
$expectedSymbolHash = 'FF1BF73872980E848ADF3EC13845BB873095BE13F43075CC196075A6F5B2E4FC'
$runningGame = Get-Process -Name 'Stalker2-Win64-Shipping' -ErrorAction SilentlyContinue
if ($runningGame) { throw 'Game is still running. Close it fully before replacing the ASI.' }
if (-not (Test-Path -LiteralPath $installedAsi)) { throw 'Installed canonical ASI was not found.' }
if (-not (Test-Path -LiteralPath $symbolAsi)) { throw 'Prepared symbol ASI was not found.' }
if ((Get-FileHash -LiteralPath $installedAsi -Algorithm SHA256).Hash -ne $expectedProductionHash) { throw 'Installed ASI hash differs from the expected production build; stopping without replacing it.' }
if ((Get-FileHash -LiteralPath $symbolAsi -Algorithm SHA256).Hash -ne $expectedSymbolHash) { throw 'Prepared symbol ASI hash mismatch; stopping.' }
if (Test-Path -LiteralPath $productionBackup) {
    if ((Get-FileHash -LiteralPath $productionBackup -Algorithm SHA256).Hash -ne $expectedProductionHash) { throw 'Existing production backup hash mismatch; stopping.' }
} else {
    Copy-Item -LiteralPath $installedAsi -Destination $productionBackup
}
Copy-Item -LiteralPath $symbolAsi -Destination $installedAsi -Force
Get-FileHash -LiteralPath $installedAsi -Algorithm SHA256
```

Після capture відновлення production ASI (також лише із закритою грою):

```powershell
$gameExe = 'E:\Steam\steamapps\common\S.T.A.L.K.E.R. 2 Heart of Chornobyl\Stalker2\Binaries\Win64\Stalker2-Win64-Shipping.exe'
$installedAsi = Join-Path (Split-Path -Parent $gameExe) 'STALKER2CameraTweaks.asi'
$productionBackup = 'E:\Work\Slaker2 mods\01-Projects\STALKER-2-Ultrawide-Fix-for-UE-5.5.4\build-artifacts\windbg-symbols\STALKER2CameraTweaks-production-backup.asi'
$expectedProductionHash = '6C76FF71D530A362B2B14F3FC1BE5EBD9D30D899495206FF885DC7258452347F'
if (Get-Process -Name 'Stalker2-Win64-Shipping' -ErrorAction SilentlyContinue) { throw 'Game is still running. Close it fully before restoring the ASI.' }
if ((Get-FileHash -LiteralPath $productionBackup -Algorithm SHA256).Hash -ne $expectedProductionHash) { throw 'Production backup hash mismatch; stopping.' }
Copy-Item -LiteralPath $productionBackup -Destination $installedAsi -Force
Get-FileHash -LiteralPath $installedAsi -Algorithm SHA256
```

## WinDbg — перший крок після заміни ASI

Новий процес потрібно запустити під WinDbg, щоб не пропустити early initialization. Не підключайся до вже запущеної гри. У WinDbg вибери **File → Start debugging → Launch executable**, вкажи `E:\Steam\steamapps\common\S.T.A.L.K.E.R. 2 Heart of Chornobyl\Stalker2\Binaries\Win64\Stalker2-Win64-Shipping.exe` і дочекайся initial breakpoint. До продовження виконай цей блок:

```text
.sympath+ "E:\Work\Slaker2 mods\01-Projects\STALKER-2-Ultrawide-Fix-for-UE-5.5.4\build-artifacts\windbg-symbols"
sxe ld:STALKER2CameraTweaks.asi
g
```

Коли WinDbg зупиниться на завантаженні ASI, виконай:

```text
.reload /f STALKER2CameraTweaks.asi
lmvm STALKER2CameraTweaks
x STALKER2CameraTweaks!*InstallSwapchainHooks*
x STALKER2CameraTweaks!*DxgiHookRegistry*Install*
x STALKER2CameraTweaks!*HookResizeBuffers*
x STALKER2CameraTweaks!*HookResizeBuffers1*
```

Переконайся, що `lmvm` показує PDB з `build-artifacts\windbg-symbols`, а чотири `x` запити знаходять symbols. Якщо ні — **не продовжуй до FG**. Надішли сюди повний output цього блоку; наступні команди будуть сформовані за фактичними symbols поточного процесу.

Якщо symbols підтверджені, поки що не вмикай FG і не став адресні breakpoints вручну. Надішли сюди цей output. Ми звіримо реальні symbol names та встановимо breakpoints точними командами, потім продовжимо до T2/T3 з адресами саме цього процесу. Жоден крок нижче не вимагає підставляти адресу в шаблон.

## Обробка зупинок — покроково в чаті

На кожній зупинці нічого не підставляй у шаблони й не продовжуй одразу. Спочатку виконай цей literal evidence block:

```text
r
~.
kv 40
ln @rip
lm a @rip
!address @rip
u @rip-10 L20
dv /t /v
```

Скопіюй повний output сюди. За конкретним hit я надам наступний набір literal команд для поточних object/vtable/slot/callback адрес. Адреси з іншого запуску не перевикористовуються. Це свідомо покрокова процедура: значення object/table, валідність slot 39, saved originals і місце для hardware watchpoints мають бути підтверджені до створення кожного наступного watchpoint.

Класифікація залишається такою: перший slot write, виконаний із `STALKER2CameraTweaks.asi` через registry publication та записавший наш callback, — це `T2_OWN_WRITE`, не T3. Пізніший write у той самий slot або зміна callback/original code — окрема подія; foreign ownership визначаємо за RIP, stack і mapping, а не за самою зміною значення чи присутністю DLL.

Hardware data breakpoints обмежені. Пріоритет для підтвердженого ResizeBuffers cycle: (1) slot 13 write, (2) callback entry code write, (3) saved-original continuation code write, (4) slot 39 write лише для підтвердженого IDXGISwapChain3. Якщо debugger не прийняв watchpoint або він не активний у writer thread context, цей edge є evidence gap. Не додавай Steam pointer watchpoint, доки module identity та live instruction, що читає конкретну pointer cell, не перевірені в цьому процесі; якщо такі докази з'являться, я дам точну команду за їхнім output.

Capture достатній після T2 власного publication, щонайменше одного наступного mutation із RIP/thread/stack/module mapping та old/new value або bytes, і зафіксованого relay/continuation до callback route. Тоді зупинись і збережи transcript; подальше відтворення не потрібне. Якщо writer не спіймано або початковий стан/watchpoint coverage відсутні — звітуємо `writer edge not captured / evidence gap`, не реконструюємо автора за фінальним snapshot і не запускаємо другий цикл лише заради заповнення прогалини.
