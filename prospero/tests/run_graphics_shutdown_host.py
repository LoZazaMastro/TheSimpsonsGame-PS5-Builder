#!/usr/bin/env python3
"""Reproduce VSync/command-processor teardown overlap using exact method bodies."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import resource
import subprocess
import tempfile

SOURCE="tools/rexglue-sdk/src/graphics/graphics_system.cpp"
BASELINE="4ae0aebca81d34233f4cffce8720dc51922f0baf"


def sha(data):
    return hashlib.sha256(data).hexdigest()


def method(text, name):
    start=text.index("void GraphicsSystem::"+name+"(")
    end=text.index("\n}\n",start)+3
    return text[start:end]


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cxx",default="c++")
    parser.add_argument("--baseline-commit",default=BASELINE)
    args=parser.parse_args()
    root=Path(__file__).resolve().parents[2]
    template=root/"prospero/tests/graphics_shutdown_host.cpp.in"
    captures=root/"prospero/validation/graphics-shutdown-v3.9-captures"
    captures.mkdir(parents=True,exist_ok=True)
    current=(root/SOURCE).read_bytes()
    baseline=subprocess.check_output(["git","show",args.baseline_commit+":"+SOURCE],cwd=root)
    report={"result":"IN_PROGRESS","hardware":"NOT_RUN","baseline_commit":args.baseline_commit,
            "baseline_source_sha256":sha(baseline),"source_sha256":sha(current),
            "template_sha256":sha(template.read_bytes()),
            "runner_sha256":sha(Path(__file__).read_bytes()),"captures":[],
            "scope":"Exact GraphicsSystem::Shutdown and MarkVblank with controlled dependency doubles",
            "limits":["No real GPU, guest interrupt, PS5 pthread or presentation ABI is exercised",
                      "An indefinitely blocked guest interrupt can still prevent orderly shutdown",
                      "The existing app shutdown watchdog remains a separate last-resort exit policy"]}
    path=root/"prospero/validation/graphics-shutdown-v3.9.json"
    resource.setrlimit(resource.RLIMIT_CORE,(0,0))
    environment=dict(os.environ,ASAN_OPTIONS="detect_leaks=0:halt_on_error=1",UBSAN_OPTIONS="halt_on_error=1")

    def run(name,command,expected=0):
        result=subprocess.run([str(x) for x in command],capture_output=True,text=True,timeout=30,env=environment)
        output=result.stdout+result.stderr
        capture=captures/(name+".txt")
        capture.write_text(output+f"\nEXIT_CODE={result.returncode}\n")
        report["captures"].append({"name":name,"command":[str(x) for x in command],"exit_code":result.returncode,
            "expected_exit_code":expected,"capture":str(capture.relative_to(root)),"capture_sha256":sha(capture.read_bytes())})
        path.write_text(json.dumps(report,indent=2)+"\n")
        print(f"{name}: exit {result.returncode} expected {expected}",flush=True)
        if result.returncode!=expected:
            raise RuntimeError(name+": "+output[-4000:])
        return output

    try:
        with tempfile.TemporaryDirectory(prefix="simpsons-graphics-shutdown-") as temp:
            work=Path(temp)
            for variant,source in [("baseline",baseline),("fixed",current)]:
                generated=template.read_text().replace("@SHUTDOWN@",method(source.decode(),"Shutdown"))
                generated=generated.replace("@MARK_VBLANK@",method(source.decode(),"MarkVblank"))
                cpp=work/(variant+".cpp"); cpp.write_text(generated)
                report[variant+"_fixture_sha256"]=sha(cpp.read_bytes())
                for suffix,flags in [("",[]),("-asan",["-fsanitize=address,undefined"])]:
                    binary=work/(variant+suffix)
                    run("compile-"+variant+suffix,[args.cxx,"-std=c++23","-O1","-g","-pthread",
                        "-fno-omit-frame-pointer",*flags,cpp,"-o",binary])
                    for case in ["overlap","empty","cp-only","vsync-only","presenter","full"]:
                        run(variant+suffix+"-"+case,[binary,case,*(["baseline"] if variant=="baseline" else [])])
                    if suffix:
                        output=run(variant+suffix+"-overlap-fault",[binary,"overlap-fault",
                            *(["baseline"] if variant=="baseline" else [])],expected=1 if variant=="baseline" else 0)
                        if variant=="baseline" and "heap-use-after-free" not in output:
                            raise RuntimeError("The baseline did not produce the expected ASan lifetime violation")
        unchanged=sha((root/SOURCE).read_bytes())==report["source_sha256"]
        report.update(result="PASS" if unchanged else "SOURCE_CHANGED_DURING_TEST",sources_unchanged_during_test=unchanged)
        if not unchanged:
            raise RuntimeError("Source changed during test")
    except Exception as error:
        report.update(result="FAIL",error=str(error)); raise
    finally:
        path.write_text(json.dumps(report,indent=2)+"\n")
    print("PASS "+str(path),flush=True)


if __name__=="__main__":
    main()
