import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.mem.MemoryBlock;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public class ValidateV4ResolverSet20261119 extends GhidraScript {
    private static final String GAMEPLAY = "F6 86 62 02 00 00 10 F3 0F 10 86 30 02 00 00 0F 85 ?? ?? ?? ?? 48 8D 4B 30 F3 0F 11 43 30 F3 0F 10 86 54 02 00 00 F3 0F 11 43 5C 0F B6 96 59 02 00 00 8B 43 68 83 E2 01 83 E0 FE 09 D0 89 43 68 0F B6 96 59 02 00 00 83 E2 04 83 E0 FB 09 D0 89 43 68 8A 96 63 02 00 00 88 53 6C";
    private static final String WRITER_POST = "44 8A 8E 61 02 00 00 44 8A 86 60 02 00 00 F3 0F 10 8E 5C 02 00 00 48 89 D9 E8 ?? ?? ?? ?? 80 BE 40 02 00 00 00";
    private static final String DIALOGUE = "48 8B 0A 48 8B 01 0F 28 CE FF 90 08 06 00 00 0F 2E 76 2C 75 02 7B 1A";
    private static final String POST_SETTER = "FF 90 08 06 00 00 0F 2E 76 2C 75 02 7B 1A";
    private static final String POST_ACTIVATION = "F3 0F 11 46 58 48 8D 4E 28 48 8B 46 28 FF 50 10";
    private static final String ZOOM_IN = "F3 0F 10 40 4C F3 0F 10 8E 38 01 00 00 0F 2E C8";
    private static final String ZOOM_OUT = "F3 0F 10 40 50 F3 0F 10 8E 3C 01 00 00 0F 2E C8";
    private static final String LANG = "41 56 56 57 53 48 83 EC 28 48 89 CE E8 ?? ?? ?? ?? 48 8B 48 28 48 8B 78 30 48 85 FF 74 ?? F0 FF 47 08 E8 ?? ?? ?? ?? 48 C7 06 00 00 00 00 48 63 58 08 4C 8B 30 89 5E 08 48 85 DB 74 ?? 48 89 F1 89 DA 45 31 C0 E8 ?? ?? ?? ?? 48 8B 0E 48 01 DB 4C 89 F2 49 89 D8 E8 ?? ?? ?? ??";
    private static final String LANG_FREE = "48 89 C1 48 C1 E1 04 49 89 34 0E 48 C1 E0 04 4D 89 6C 06 08 4C 01 AF 38 15 00 00 FF 87 40 15 00 00";
    private static final String ASPECT = "48 8B 41 18 48 8B 88 F8 00 00 00 80 89 59 02 00 00 01 48 8B 80 F8 00 00 00 C7 80 54 02 00 00 39 8E E3 3F B0 01 C3";
    private static final String ENTER = "F3 0F 10 05 ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 89 F1 E8 ?? ?? ?? ?? 48 89 C1 31 D2 E8 ?? ?? ?? ??";
    private static final String EXIT = "F3 0F 10 47 38 E8 ?? ?? ?? ?? 48 89 F1 E8 ?? ?? ?? ?? 48 89 C1 31 D2 E8 ?? ?? ?? ??";
    private static final String EXIT_INDEXED = "40 0F B6 C7 F3 0F 10 44 83 38 E8 ?? ?? ?? ?? 48 89 F1 E8 ?? ?? ?? ?? 48 89 C1 31 D2 E8 ?? ?? ?? ?? 48 85 C0 74 ?? 48 89 C7 48 8B 00 48 89 F9 FF 90 80 08 00 00 48 8B 07 48 89 F9 FF 90 68 08 00 00";
    private static final String ENTER_PAIR = "FF 90 78 08 00 00 48 8B 07 48 89 F9 B2 01 FF 90 60 08 00 00";
    private static final String EXIT_PAIR = "FF 90 80 08 00 00 48 8B 07 48 89 F9 FF 90 68 08 00 00";

    private static class Hit { String name; long count; Address address; Hit(String n,long c,Address a){name=n;count=c;address=a;} }
    private byte[] bytes(String s){String[] p=s.split(" ");byte[] b=new byte[p.length];for(int i=0;i<p.length;i++)b[i]=(byte)(p[i].equals("??")?0:Integer.parseInt(p[i],16));return b;}
    private byte[] mask(String s){String[] p=s.split(" ");byte[] b=new byte[p.length];for(int i=0;i<p.length;i++)b[i]=(byte)(p[i].equals("??")?0:255);return b;}
    private Hit scan(String name,String pattern,MemoryBlock text)throws Exception{byte[] b=bytes(pattern),m=mask(pattern);long n=0;Address first=null;if(text.getSize()>=b.length){Address end=text.getEnd().subtract(b.length-1);Address h=currentProgram.getMemory().findBytes(text.getStart(),end,b,m,true,monitor);while(h!=null){n++;if(first==null)first=h;h=h.equals(end)?null:currentProgram.getMemory().findBytes(h.add(1),end,b,m,true,monitor);}}return new Hit(name,n,first);}
    private boolean contains(Address start,int span,String pattern,MemoryBlock text)throws Exception{byte[] b=bytes(pattern),m=mask(pattern);if(!text.contains(start)||!text.contains(start.add(span-1)))return false;Address end=start.add(span-1);return currentProgram.getMemory().findBytes(start,end,b,m,true,monitor)!=null;}
    private String rva(Address a){return a==null?"-":"0x"+Long.toHexString(a.subtract(currentProgram.getImageBase()));}
    private long callTarget(Address call)throws Exception{byte[] b=new byte[4];currentProgram.getMemory().getBytes(call.add(1),b);int d=ByteBuffer.wrap(b).order(ByteOrder.LITTLE_ENDIAN).getInt();return call.add(5).add((long)d).getOffset();}
    private String expectedHash(String name){if(name.equalsIgnoreCase("Stalker2-Win64-Shipping.exe"))return "2bd3f115b959a8eee7e5c4b42b0142dd0025bd437e1014f92dfc60b9afee5479";if(name.contains("2.0.2"))return "f0757ebe01f3d085d88ab01d08b0c6d449301981aaa45c0678cf93fbf5b6cc0c";if(name.contains("2.0.3"))return "81961b7281c7cf528ce49c549ce086fcc684bd676f32faf042bc743d939e3c69";if(name.contains("2.0.4"))return "2ecc5d19fe37f97e3f7f2467d652b299b5a47f010fa49fd803a49a4a6930a409";if(name.contains("2.0.5"))return "e7b481a97c02d80581fab0bece940214a88ebe30211088a00129845a039f9293";if(name.contains("2.0.6"))return "61bc1e030740cebc30cf1dad0c86cf65e39e12ff0500225821d684181e08d56b";return null;}
    @Override public void run()throws Exception{
        String name=currentProgram.getDomainFile().getName();String expected=expectedHash(name);String actual=currentProgram.getExecutableSHA256();
        println("PROGRAM="+name+" HASH="+actual+" EXPECTED="+expected+" IMAGE_BASE="+currentProgram.getImageBase());
        if(expected==null||!expected.equalsIgnoreCase(actual)||currentProgram.getImageBase().getOffset()!=0x140000000L){println("IDENTITY=STOP");return;}
        MemoryBlock text=null;for(MemoryBlock b:currentProgram.getMemory().getBlocks())if(b.getName().equals(".text"))text=b;
        if(text==null||!text.isExecute()){println("TEXT=STOP");return;}println("IDENTITY=PASS TEXT_START="+text.getStart()+" TEXT_SIZE="+text.getSize());
        List<Hit> h=new ArrayList<>();h.add(scan("GAMEPLAY_WRITER",GAMEPLAY,text));h.add(scan("WRITER_POSTPROCESS_DIAGNOSTIC",WRITER_POST,text));h.add(scan("DIALOGUE",DIALOGUE,text));h.add(scan("POST_SETTER_DIAGNOSTIC",POST_SETTER,text));h.add(scan("POST_EXIT_ACTIVATION",POST_ACTIVATION,text));h.add(scan("ZOOM_IN",ZOOM_IN,text));h.add(scan("ZOOM_OUT",ZOOM_OUT,text));h.add(scan("LANGUAGE_READER",LANG,text));h.add(scan("LANGUAGE_FREE_ANCHOR",LANG_FREE,text));h.add(scan("CINEMATIC_ASPECT",ASPECT,text));h.add(scan("CINEMATIC_ENTER",ENTER,text));h.add(scan("CINEMATIC_EXIT_LEGACY",EXIT,text));h.add(scan("CINEMATIC_EXIT_INDEXED",EXIT_INDEXED,text));
        for(Hit x:h)println(x.name+"="+x.count+" RVA="+rva(x.address));
        Hit g=h.get(0),ent=h.get(10),old=h.get(11),idx=h.get(12),dlg=h.get(2),act=h.get(4),zin=h.get(5),zout=h.get(6);
        byte[] writerInstruction=new byte[5];if(g.count==1)currentProgram.getMemory().getBytes(g.address.add(25),writerInstruction);boolean gameDecode=g.count==1&&Arrays.equals(writerInstruction,new byte[]{(byte)0xF3,0x0F,0x11,0x43,0x30});
        println("GAMEPLAY_INSTRUCTION_CONTRACT="+(gameDecode?"PASS":"FAIL"));
        boolean enterExit=false;Address chosen=null;int callOffset=0;
        if(ent.count==1){if(old.count==1){chosen=old.address;callOffset=5;}else if(old.count==0&&idx.count==1){chosen=idx.address;callOffset=10;}}
        if(ent.count==1&&chosen!=null){long et=callTarget(ent.address.add(8)),xt=callTarget(chosen.add(callOffset));enterExit=et==xt&&contains(ent.address,96,ENTER_PAIR,text)&&contains(chosen,96,EXIT_PAIR,text);}
        println("CINEMATIC_CALL_AND_VCALL_CONTRACT="+(enterExit?"PASS":"FAIL")+" selectedExit="+(old.count==1?"legacy":(old.count==0&&idx.count==1?"indexed":"none")));
        println("CORE_REQUIRED="+((gameDecode&&h.get(9).count==1&&enterExit&&dlg.count==1)?"PASS":"FAIL"));
        println("DIALOGUE_PROVIDER_ACTIVATION="+(act.count==1?"PRESENT":"ABSENT"));
        println("ZOOM_PAIR="+(zin.count==1&&zout.count==1?"PRESENT":"ABSENT"));
        println("OPTIONAL_DIAGNOSTICS="+((h.get(1).count==1&&h.get(3).count==1)?"POSTPROCESS_AND_SETTER_PRESENT":"PARTIAL_OR_ABSENT"));
        println("LANGUAGE_RESOLVER="+((h.get(7).count==1&&h.get(8).count==1)?"PRESENT":"ABSENT_OR_PARTIAL"));
    }
}
