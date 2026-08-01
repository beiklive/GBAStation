### Compilation (Linux)

```
mkdir build && cd build
cmake -DPLATFORM_LINUX=ON -DBUILD_SSCRAP=ON ..
make -j4
```

### Usage

scrap mame/fbneo system, download 'mixrbv2' for 'image', 'box-3D' for 'thumbnail' and 'video' for 'video':
```
sscrap -u user -p password -r /roms -sid 75 -i mixrbv2 -t box-3D -v video
```

### Help / Options
```
usage: sscrap -u <screenscraper_user> -p <screenscraper_password> [options]

	options:
		-d                             enable debug output
		-sl                            list available screenscraper systems and exit
		-ml                            list available screenscraper medias types and exit
		-zi <rom_path>                 show zip information (size, crc, md5, sha1) and exit
		-sid <system_id>               screenscraper system id to scrap
		-r <roms_path>                 path to roms files to scrap
		-i <mediaType>                 use given media type for image
		-t <mediaType>                 use given media type for thumbnail
		-v <mediaType>                 use given media type for video
		-c                             download medias for clones (else use parent)
		-filter <ext>                  only scrap files with this extension

	sscrap customs systemid (fbneo):
		750: ColecoVision (fbneo)
		751: Game Gear (fbneo)
		752: Master System (fbneo)
		753: Megadrive (fbneo)
		754: MSX 1 (fbneo)
		755: NEC PC-Engine (fbneo)
		756: Sega SG-1000 (fbneo)
		757: NEC Super Grafx (fbneo)
		758: NEC Turbo Grafx (fbneo)
		759: ZX Spectrum (fbneo)
		760: Nintendo NES (fbneo)
		761: Nintendo NES FDS (fbneo)
		762: Fairchild Channel F (fbneo)
		763: NeoGeo Pocket (fbneo)
```
