#!/usr/bin/env python3
"""Read-only linked/native ELF anchor verification; never executes an image."""
import argparse,hashlib,json,re,struct,subprocess
from pathlib import Path

def info(path):
    b=path.read_bytes()
    assert b[:6]==b"\x7fELF\x02\x01"
    entry,phoff,shoff=struct.unpack_from("<QQQ",b,24)
    ps,pc,ss,sc=struct.unpack_from("<HHHH",b,54)
    assert ps==56
    ph=[struct.unpack_from("<IIQQQQQQ",b,phoff+i*ps) for i in range(pc)]
    def readva(addr,size):
        m=[h for h in ph if h[0]==1 and h[3]<=addr and addr+size<=h[3]+h[5]]
        assert len(m)==1,(hex(addr),size)
        h=m[0];start=h[2]+addr-h[3]
        return b[start:start+size]
    rel=[]
    for h in ph:
        if h[0]!=2:continue
        tags={}
        for x in range(h[2],h[2]+h[5],16):
            tag,value=struct.unpack_from("<qQ",b,x)
            if not tag:break
            tags[tag]=value
        for a,z in [(7,8),(23,2)]:
            if tags.get(z,0):
                data=readva(tags[a],tags[z]);assert len(data)%24==0
                rel.extend(struct.unpack_from("<QQq",data,i) for i in range(0,len(data),24))
    syms={}
    if shoff:
        sh=[struct.unpack_from("<IIQQQQIIQQ",b,shoff+i*ss) for i in range(sc)]
        for h in sh:
            if h[1]!=2:continue
            strings=sh[h[6]];names=b[strings[4]:strings[4]+strings[5]]
            for x in range(h[4],h[4]+h[5],h[9]):
                n,si,vis,sec,val,size=struct.unpack_from("<IBBHQQ",b,x)
                name=names[n:names.find(b"\0",n)].decode()
                if name in ("_start","__prospero_linked_entry"):
                    syms[name]={"value":val,"size":size,"binding":si>>4,"type":si&15,"section":sec}
    return b,entry,readva,rel,syms

ap=argparse.ArgumentParser();ap.add_argument("linked",type=Path);ap.add_argument("--native",type=Path);ap.add_argument("--report",type=Path,required=True);a=ap.parse_args()
b,entry,readva,rel,syms=info(a.linked)
start=syms["_start"];anchor=syms["__prospero_linked_entry"]
assert start["value"]==entry==anchor["value"]
assert anchor["section"]==0xfff1 and anchor["binding"]==0
text=subprocess.check_output(["llvm-objdump","-d","--disassemble-symbols=_start",str(a.linked)],text=True)
a.report.with_suffix(".disassembly.txt").write_text(text)
sites=[]
for line in text.splitlines():
    m=re.match(r"\s*([0-9a-f]+):\s+((?:[0-9a-f]{2}\s+)+)movabsq\s+\$(0x[0-9a-f]+),",line)
    if m and int(m[3],16)==entry:
        addr=int(m[1],16);code=bytes.fromhex(m[2]);assert len(code)==10
        assert code[0] in (0x48,0x49) and 0xb8<=code[1]<=0xbf
        assert int.from_bytes(code[2:],"little")==entry
        assert readva(addr,len(code))==code
        assert not any(off<addr+len(code) and off+8>addr+2 for off,_,_ in rel)
        sites.append({"instruction_va":hex(addr),"immediate_va":hex(addr+2),"bytes":code.hex(),"value":hex(entry)})
assert sites,"no verified movabs linked-entry instruction in _start"
r={"status":"PASS","scope":"static ELF inspection; no title execution","linked_elf":str(a.linked),"linked_sha256":hashlib.sha256(b).hexdigest(),"entry":hex(entry),"symbols":syms,"constant_sites":sites,"linked_dynamic_relocation_count":len(rel),"constant_has_dynamic_relocation":False,"start_body_bytes":start["size"]}
if a.native:
    nb,ne,nread,nrel,ns=info(a.native)
    assert ne==entry
    assert nread(start["value"],start["size"])==readva(start["value"],start["size"])
    for site in sites:
        addr=int(site["instruction_va"],16);code=bytes.fromhex(site["bytes"])
        assert nread(addr,len(code))==code
        assert not any(off<addr+len(code) and off+8>addr+2 for off,_,_ in nrel)
    r.update(native_elf=str(a.native),native_sha256=hashlib.sha256(nb).hexdigest(),native_entry=hex(ne),native_start_body_identical=True,native_dynamic_relocation_count=len(nrel),native_constant_has_dynamic_relocation=False)
a.report.write_text(json.dumps(r,indent=2)+"\n")
print(json.dumps(r,indent=2))
