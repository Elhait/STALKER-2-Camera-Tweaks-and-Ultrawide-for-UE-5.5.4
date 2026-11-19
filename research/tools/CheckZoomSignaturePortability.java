// Read-only cardinality check. SHA identifies evidence; it never selects admission.
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.mem.MemoryBlock;

public class CheckZoomSignaturePortability extends GhidraScript {
    private long count(String label, String signature) throws Exception {
        String[] tokens = signature.split(" ");
        byte[] bytes = new byte[tokens.length];
        for (int i = 0; i < tokens.length; i++) bytes[i] = (byte)Integer.parseInt(tokens[i], 16);
        long count = 0;
        for (MemoryBlock block : currentProgram.getMemory().getBlocks()) {
            if (!block.isExecute() || !block.isInitialized() || block.getSize() < bytes.length) continue;
            Address end = block.getEnd().subtract(bytes.length - 1);
            Address hit = currentProgram.getMemory().findBytes(block.getStart(), end, bytes, null, true, monitor);
            while (hit != null) {
                count++;
                if (count <= 8) println(label + "_RVA=" + Long.toHexString(hit.subtract(currentProgram.getImageBase())));
                hit = hit.equals(end) ? null : currentProgram.getMemory().findBytes(hit.add(1), end, bytes, null, true, monitor);
            }
        }
        return count;
    }
    public void run() throws Exception {
        println("ZOOM_IMAGE SHA256=" + currentProgram.getExecutableSHA256());
        long in = count("IN", "F3 0F 10 40 4C F3 0F 10 8E 38 01 00 00 0F 2E C8");
        long out = count("OUT", "F3 0F 10 40 50 F3 0F 10 8E 3C 01 00 00 0F 2E C8");
        println("ZOOM_MATCHES in=" + in + " out=" + out + " unique=" + (in == 1 && out == 1));
    }
}
