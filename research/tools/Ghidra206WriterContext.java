// Read-only inspection of the existing analyzed 2.0.6 program.
// Run with analyzeHeadless -process -readOnly -noanalysis.
// @category STALKER2

import ghidra.app.script.GhidraScript;
import ghidra.framework.options.Options;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.Listing;
import ghidra.program.model.symbol.Reference;
import ghidra.program.model.symbol.ReferenceIterator;

public class Ghidra206WriterContext extends GhidraScript {
    private static final long WRITER_RVA = 0xA9F8CB;

    @Override
    public void run() throws Exception {
        println("PROGRAM=" + currentProgram.getName());
        println("IMAGE_BASE=" + currentProgram.getImageBase());
        println("MIN_ADDRESS=" + currentProgram.getMinAddress());
        println("MAX_ADDRESS=" + currentProgram.getMaxAddress());
        Options info = currentProgram.getOptions("Program Information");
        for (String name : info.getNames()) {
            try {
                String value = info.getString(name, "");
                if (!value.isEmpty()) println("PROGRAM_INFO " + name + "=" + value);
            }
            catch (Exception ignored) {
                // Ignore non-string properties.
            }
        }

        Address writer = currentProgram.getImageBase().add(WRITER_RVA);
        Function function = currentProgram.getFunctionManager().getFunctionContaining(writer);
        println("WRITER=" + writer + " FUNCTION=" + functionName(function));
        Listing listing = currentProgram.getListing();
        dump(listing, writer, 24, 32, "WRITER");

        if (function == null) return;
        ReferenceIterator references = currentProgram.getReferenceManager()
            .getReferencesTo(function.getEntryPoint());
        while (references.hasNext()) {
            Reference reference = references.next();
            if (!reference.getReferenceType().isCall()) continue;
            Address call = reference.getFromAddress();
            Function caller = currentProgram.getFunctionManager().getFunctionContaining(call);
            println("CALLER_REF=" + call + " FUNCTION=" + functionName(caller));
            dump(listing, call, 20, 28, "CALLER");
        }
    }

    private String functionName(Function function) {
        return function == null ? "<none>" :
            function.getName() + "@" + function.getEntryPoint();
    }

    private void dump(Listing listing, Address pivot, int before, int after, String label) {
        Instruction at = listing.getInstructionAt(pivot);
        if (at == null) at = listing.getInstructionAfter(pivot.subtract(1));
        if (at == null) {
            println(label + " <no instruction>");
            return;
        }
        Instruction first = at;
        for (int i = 0; i < before; i++) {
            Instruction previous = listing.getInstructionBefore(first.getAddress());
            if (previous == null) break;
            first = previous;
        }
        Instruction instruction = first;
        for (int i = 0; instruction != null && i < before + after + 1; i++) {
            println(label + " " + instruction.getAddress() + " " +
                instruction.getDefaultRepresentation());
            instruction = listing.getInstructionAfter(instruction.getAddress());
        }
    }
}
