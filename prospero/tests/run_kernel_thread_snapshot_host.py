#!/usr/bin/env python3
"""Exercise exact title termination/lookup bodies with production object_ref."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import resource
import signal
import subprocess
import tempfile

BASELINE="4ae0aebca81d34233f4cffce8720dc51922f0baf"
SOURCE="tools/rexglue-sdk/src/system/kernel_state.cpp"
XOBJECT="tools/rexglue-sdk/include/rex/system/xobject.h"
TABLE="tools/rexglue-sdk/src/system/util/object_table.cpp"
TABLE_HEADER="tools/rexglue-sdk/include/rex/system/util/object_table.h"
REF_IMPL="tools/rexglue-sdk/src/system/xobject.cpp"


def sha(data):
    return hashlib.sha256(data).hexdigest()


def extract(text,signature):
    start=text.index(signature)
    return text[start:text.index("\n}\n",start)+3]


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cxx",default="c++")
    parser.add_argument("--baseline-commit",default=BASELINE)
    args=parser.parse_args()
    root=Path(__file__).resolve().parents[2]
    fixture=root/"prospero/tests/kernel_thread_snapshot_host.cpp.in"
    capture_root=root/"prospero/validation/kernel-thread-snapshot-v3.9-captures"
    capture_root.mkdir(parents=True,exist_ok=True)
    path=root/"prospero/validation/kernel-thread-snapshot-v3.9.json"
    inputs=[SOURCE,XOBJECT,TABLE,TABLE_HEADER,REF_IMPL,
            "prospero/tests/kernel_thread_snapshot_host.cpp.in","prospero/tests/run_kernel_thread_snapshot_host.py"]
    hashes={p:sha((root/p).read_bytes()) for p in inputs}
    baseline=subprocess.check_output(["git","show",args.baseline_commit+":"+SOURCE],cwd=root)
    object_header=(root/XOBJECT).read_text()
    object_ref=object_header[object_header.index("template <typename T>\nclass object_ref {"):
                             object_header.index("template <typename T>\nobject_ref<T> XObject::GetNativeObject")]
    table_header=(root/TABLE_HEADER).read_text()
    begin=table_header.index("  template <typename T>\n  std::vector<object_ref<T>> GetObjectsByType()")
    table_template=table_header[begin:table_header.index("\n  }\n",begin)+5]
    xobject=(root/REF_IMPL).read_text()
    ref_impl=extract(xobject,"void XObject::Retain()")+"\n"+extract(xobject,"void XObject::Release()")
    report={"result":"IN_PROGRESS","hardware":"NOT_RUN","baseline_commit":args.baseline_commit,
            "baseline_source_sha256":sha(baseline),"reviewed_source_sha256":hashes,"captures":[],
            "scope":"Exact TerminateTitle/GetThreadByID/UnregisterThread, production object_ref and ObjectTable enumeration; controlled object lifetime, unlock injection and native cleanup on a separate pthread",
            "limits":["No game, PS5 native execution, scheduling or guest cancellation is exercised",
                      "Controlled release on the actual unlock boundary proves ownership, not all interleavings",
                      "Self exit uses siglongjmp with heap-owned test state to bypass automatic destructors",
                      "The fixture models object-table ownership and uses its actual enumeration body; guest handle numbering is not modeled",
                      "Wait is a controlled dependency that releases and awaits a native worker needing the real test global mutex; the full ReX Wait/stack integration is checked by thread-ownership-v3.9.json"]}
    resource.setrlimit(resource.RLIMIT_CORE,(0,0))

    def run(name,command,expected=0):
        env=dict(os.environ,ASAN_OPTIONS="detect_leaks=0:halt_on_error=1",UBSAN_OPTIONS="halt_on_error=1")
        result=subprocess.run([str(x) for x in command],capture_output=True,text=True,timeout=30,env=env)
        output=result.stdout+result.stderr
        capture=capture_root/(name+".txt")
        capture.write_text(output+f"\nEXIT_CODE={result.returncode}\n")
        report["captures"].append({"name":name,"command":[str(x) for x in command],"exit_code":result.returncode,
             "expected_exit_code":expected,"capture":str(capture.relative_to(root)),"capture_sha256":sha(capture.read_bytes())})
        path.write_text(json.dumps(report,indent=2)+"\n")
        print(f"{name}: exit {result.returncode} expected {expected}",flush=True)
        if result.returncode!=expected: raise RuntimeError(name+": "+output[-6000:])
        return output

    try:
        with tempfile.TemporaryDirectory(prefix="simpsons-kernel-thread-snapshot-") as temp:
            work=Path(temp)
            for variant,source in [("baseline",baseline),("fixed",(root/SOURCE).read_bytes())]:
                generated=fixture.read_text()
                substitutions={
                    "@RETAIN_RELEASE@":ref_impl,"@OBJECT_REF@":object_ref,
                    "@GET_OBJECTS_TEMPLATE@":table_template,
                    "@GET_OBJECTS@":extract((root/TABLE).read_text(),"void ObjectTable::GetObjectsByType("),
                    "@TERMINATE_TITLE@":extract(source.decode(),"void KernelState::TerminateTitle()"),
                    "@GET_THREAD_BY_ID@":extract(source.decode(),"object_ref<XThread> KernelState::GetThreadByID("),
                    "@UNREGISTER_THREAD@":extract(source.decode(),"void KernelState::UnregisterThread(")}
                for tag,value in substitutions.items(): generated=generated.replace(tag,value)
                cpp=work/(variant+".cpp"); cpp.write_text(generated)
                report[variant+"_generated_sha256"]=sha(cpp.read_bytes())
                variants=[("-asan",["-fsanitize=address,undefined"])] if variant=="baseline" else [
                    ("",[]),("-asan",["-fsanitize=address,undefined"]),("-no-unwind",["-fno-exceptions"])]
                for suffix,flags in variants:
                    binary=work/(variant+suffix)
                    run("compile-"+variant+suffix,[args.cxx,"-std=c++23","-O1","-g","-pthread",
                        "-fno-omit-frame-pointer",*flags,cpp,"-o",binary])
                    for case in ["overlap","duplicates","self-exit","lookup-destructing",
                                 "lookup-destructing-fault","lookup","replaced-id","filter",
                                 "delayed-cleanup","startup-suspended","finished-cleanup","wait-failure"]:
                        expected=1 if variant=="baseline" and case in ["overlap","lookup-destructing-fault"] else 0
                        if variant=="fixed" and case=="wait-failure": expected=-signal.SIGABRT
                        output=run(variant+suffix+"-"+case,[binary,case,*(["baseline"] if variant=="baseline" else [])],expected)
                        if expected==1 and not any(marker in output for marker in ["heap-use-after-free","runtime error: member call"]):
                            raise RuntimeError("Baseline failed without identifying the expected object lifetime violation")
        unchanged=all(sha((root/p).read_bytes())==h for p,h in hashes.items())
        report.update(result="PASS" if unchanged else "SOURCE_CHANGED_DURING_TEST",sources_unchanged_during_test=unchanged)
        if not unchanged: raise RuntimeError("Production source changed during test")
    except Exception as error:
        report.update(result="FAIL",error=str(error)); raise
    finally:
        path.write_text(json.dumps(report,indent=2)+"\n")
    print("PASS "+str(path),flush=True)


if __name__=="__main__": main()
