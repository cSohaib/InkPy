# TJpgDec provenance

ChaN TJpgDec R0.03 (2021), copied from Espressif idf-extra-components/esp_jpeg/tjpgd.
Original permissive copyright/licence notice remains in tjpgd.c.

Source: https://github.com/espressif/idf-extra-components/tree/master/esp_jpeg/tjpgd
Pinned Git blobs:
- tjpgd.c: 804f0ecf129c8eb11f40e3185ac5c8b27bc6d4ed
- tjpgd.h: 8181b080a7cd4ad6cf5f804b6216abfff6a682d2
- upstream tjpgdcnf.h: fa6a28c9150b9d8a4c9b095d1683d4379d493104

C/header retained; configuration replaced with fixed RGB888 software-decoder
settings: SZBUF=512, FORMAT=0, USE_SCALE=1, TBLCLIP=0, FASTDECODE=0.
The same decoder is compiled on host and device. No ROM decoder dependency.
