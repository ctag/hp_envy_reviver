HP Device Firmware Update Tool -- READ ME
===========================================================

This tool is used to update/reovery/rollback device firmware for HP projects.

===========================================================
NOTE
===========================================================
For the platform with 2013 secure key, system will launch HpDevFwUpdate.EFI.
For the platform with 2017 secure key, system will launch DevFwUpdate.EFI.

      Key       |  64bit Tool Name  |   32bit Tool Name
================|===================|=====================
2013 secure key | HpDevFwUpdate.efi | HpDevFwUpdate32.efi
----------------|-------------------|---------------------
2017 secure key |   DevFwUpdate.efi |   DevFwUpdate32.efi

===========================================================
File List
===========================================================
This package including:

HpDevFwUpdate.efi    -- (v1.0.3.0) The 64 bit version main executable file. (sign with 2013 secure key)
HpDevFwUpdate.s12    -- (v1.0.3.0) The sign file of 64 bit version main executable file.
HpDevFwUpdate32.efi  -- (v1.0.3.0) The 32 bit version main executable file. (sign with 2013 secure key)
HpDevFwUpdate32.s12  -- (v1.0.3.0) The sign file of 32 bit version main executable file.
DevFwUpdate.efi      -- The 64 bit version main executable file. (sign with 2017 secure key)
DevFwUpdate.s12      -- The sign file of 64 bit version main executable file.
DevFwUpdate32.efi    -- The 32 bit version main executable file. (sign with 2017 secure key)
DevFwUpdate32.s12    -- The sign file of 32 bit version main executable file.
Doc\Readme.txt       -- This file. The brief description and usage of flash utility.
Doc\ReleaseNote.txt  -- Release history.

===========================================================
Command usage:
===========================================================
/U  Update   - Update the device firmware in DEVFW\New\
/R  Rollback - Update the device firmware in DEVFW\Previous\
/C  reCovery - Update the device firmware in DEVFW\Current\
