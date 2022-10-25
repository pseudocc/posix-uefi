POSIX-UEFI Segédeszközök
========================

Ezek kis parancsok, amik besegítenek az UEFI toolchain-edbe. A POSIX-UEFI által lefordított .efi kimeneti fájlokat konvertálják
olyan különböző formátumú fájlokká, amiket az UEFI firmware használ.

* __efirom__ - ez PCI Option ROM képet készít

* __efiffs__ - ez DXE UEFI eszközmeghajtó képet (bővebb infó [ebben a wikiben](https://github.com/pbatard/efifs/wiki/Adding-a-driver-to-a-UEFI-firmware#adding-the-module-to-the-firmware) található arról, hogy kell a firmwarehez adni).

* __efidsk__ - indítható lemezképet készít EFI Rendszer Partícióval egy könyvtár tartalmából
