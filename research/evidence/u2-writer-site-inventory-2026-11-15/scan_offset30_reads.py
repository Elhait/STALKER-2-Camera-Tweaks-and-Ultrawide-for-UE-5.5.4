import bisect, hashlib, mmap, struct, sys, zipfile
from pathlib import Path
sys.stdout.reconfigure(encoding='utf-8')
exe=Path(r'E:\Steam\steamapps\common\S.T.A.L.K.E.R. 2 Heart of Chornobyl\Stalker2\Binaries\Win64\Stalker2-Win64-Shipping.exe')
expected='2bd3f115b959a8eee7e5c4b42b0142dd0025bd437e1014f92dfc60b9afee5479'
root=Path(r'E:\Work\Slaker2 mods\02-Research\evidence\post-exit-dataflow-2026-11-11\decoder')
wheel=Path(r'E:\Work\Slaker2 mods\Instruments\ghidra_12.1.3_PUBLIC\Ghidra\Debug\Debugger-agent-dbgeng\pypkg\dist\capstone-5.0.6-py3-none-win_amd64.whl')
if not (root/'capstone'/'__init__.py').exists():
 with zipfile.ZipFile(wheel) as z:
  for n in z.namelist():
   if n.startswith('capstone/') and '..' not in Path(n).parts: z.extract(n,root)
sys.path.insert(0,str(root))
from capstone import Cs,CS_ARCH_X86,CS_MODE_64,CS_AC_READ,CS_AC_WRITE
from capstone.x86_const import X86_OP_MEM
with exe.open('rb') as f, mmap.mmap(f.fileno(),0,access=mmap.ACCESS_READ) as d:
 h=hashlib.sha256(d).hexdigest(); print('SHA256='+h)
 if h!=expected: raise SystemExit('IMAGE_IDENTITY_MISMATCH')
 pe=struct.unpack_from('<I',d,0x3c)[0]; n=struct.unpack_from('<H',d,pe+6)[0]; osz=struct.unpack_from('<H',d,pe+20)[0]; op=pe+24
 base=struct.unpack_from('<Q',d,op+24)[0]; sec=[]
 for i in range(n):
  q=op+osz+i*40; name=d[q:q+8].rstrip(b'\0').decode(errors='replace'); vs,rva,rs,raw=struct.unpack_from('<IIII',d,q+8); sec.append((name,rva,vs,raw,rs))
 text=next(x for x in sec if x[0]=='.text'); pdata=next(x for x in sec if x[0]=='.pdata');
 ranges=[]
 for p in range(pdata[3],pdata[3]+pdata[4],12):
  b,e,_=struct.unpack_from('<III',d,p)
  if b and e>b: ranges.append((b,e))
 ranges.sort(); starts=[x[0] for x in ranges]; dec=Cs(CS_ARCH_X86,CS_MODE_64); dec.detail=True
 hits=[]
 for b,e in ranges:
  if b<text[1] or e>text[1]+text[2]: continue
  raw=text[3]+b-text[1]; size=e-b
  for ins in dec.disasm(d[raw:raw+size],base+b):
   for o in ins.operands:
    if o.type==X86_OP_MEM and o.mem.disp in (0x30,0x230):
     access=o.access
     if o.mem.disp==0x30 and access & CS_AC_READ:
      hits.append((ins.address-base,b,e,o.mem.base,ins.mnemonic,ins.op_str))
 print('READS_DISP_30',len(hits))
 funcs={}
 for r in hits: funcs[r[1]]=funcs.get(r[1],0)+1
 print('FUNCTIONS_WITH_READS',len(funcs))
 print('TOP_FUNCTIONS')
 for b,c in sorted(funcs.items(),key=lambda x:(-x[1],x[0]))[:100]: print(f'{b:#x} reads={c}')
 print('WRITER_FUNCTION_READS')
 for x in hits:
  if x[1]==0x9cd16c: print(f'{x[0]:#x} base={x[3]} {x[4]} {x[5]}')

