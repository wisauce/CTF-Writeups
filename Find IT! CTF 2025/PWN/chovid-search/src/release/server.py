import tempfile
import base64
import binascii
import os
import sys


def main():
    try:
        b64 = input("Base64 encoded file: ").strip()
    except EOFError:
        return

    try:
        x = base64.b64decode(b64)
    except binascii.Error:
        print("Invalid input", flush=True)
        return

    if len(x) >= 10**7:
        print("Invalid input", flush=True)
        return

    with tempfile.NamedTemporaryFile() as f:
        f.write(x)
        f.seek(0)

        try:
            os.system(" ".join([
                "./qemu-system-x86_64",
                "-nographic",
                "-nodefaults",
                "-net", "none",
                "-serial", "stdio",
                "-m", "128",
                "-L", "/usr/share/seabios/",
                "-L", "/usr/lib/ipxe/qemu/",
                "-device", "chovidsearch,id=chovidsearch",
                "-drive", f"format=raw,file={f.name},snapshot=on,index=2",
                "-kernel", "bzImage",
                "-initrd", "initramfs.cpio.gz",
                "-append", "console=ttyS0",
                "-option-rom", "/usr/share/qemu/kvmvapic.bin",
                "-option-rom", "/usr/share/qemu/linuxboot_dma.bin"
            ]))
        except Exception as e:
            print(e, flush=True)


if __name__ == "__main__":
    main()