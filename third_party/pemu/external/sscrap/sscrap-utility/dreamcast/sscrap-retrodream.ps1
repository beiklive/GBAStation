# 1 - you need to enable script execution from powershell: "Set-ExecutionPolicy Unrestricted"
# 2 - you need an account on screenscraper to be able to scrap => https://www.screenscraper.fr/
# 3 - set you screenscraper username and password below...
$user = "xxx"
$password = "xxx"

function write-error($msg){
    $msg | write-host -fore green -back red;
}

$games_path = "GAMES/"
$media_path = "RD/medias/"

if(![string]::IsNullOrWhiteSpace($args[0])) {
    $games_path = $args[0];
}

if(![string]::IsNullOrWhiteSpace($args[1])) {
    $media_path = $args[1];
}

if(-not(Test-Path $games_path)) {
    write-error "Error: game path doesn't exist: $games_path"
    exit 1
}

# set paths
New-Item $media_path -ItemType Directory -Force
$games_path = Resolve-Path($games_path)
$media_path = Resolve-Path($media_path)
Write-Output "games_path: $games_path"
Write-Output "media_path: $media_path"

# sscrap
.\sscrap\sscrap-utility.exe -u $user -p $password -sid 23 -r $games_path -m $media_path -dlmc -dlm video-normalized box-2D


# optimise png
##############
add-type -AssemblyName System.Drawing

Write-Output "`nImage conversion (imagemagick/pngquant)`n======================================`n"
Get-ChildItem -Path $media_path -Filter *.png -Recurse -ErrorAction SilentlyContinue -Force |
        Foreach-Object {
            $file = $_.FullName
            $img = [Drawing.Image]::FromFile($file)
            $w = $img.Width
            $h = $img.Height
            $img.Dispose()
            if(($w -gt 256) -or ($h -gt 256)) {
                Write-Output "processing: $file ($($w)x$($h) > 256x256)"
                .\sscrap\magick.exe mogrify -resize 256x256 $file
                .\sscrap\pngquant.exe --ext .png --force 256 $file
            } else {
                Write-Output "skipped: $file (already converted)"
            }
        }

# convert videos
################
Write-Output "`nVideo conversion (ffmpeg)`n==========================`n"
Get-ChildItem -Path $media_path -Filter *.mp4 -Recurse -ErrorAction SilentlyContinue -Force |
        Foreach-Object {
            $path = $_.Directory
            $file = [System.Io.Path]::GetFileNameWithoutExtension($_.Name);
            $input = "$path\$file.mp4"
            $output = "$path\$file.roq"
            if(![System.IO.File]::Exists($output)) {
                Write-Output "processing: $file.mp4 > $file.roq"
                .\sscrap\ffmpeg.exe -v 8 -i $input -ar 22050 -framerate 30 -vf "scale=256:-2" -t 30 $output
            } else {
                Write-Output "skipped: $input (already converted)"
            }
        }
Get-ChildItem -Path $media_path -Include *.mp4 -Recurse | Remove-Item

# done...
##########
Write-Host -NoNewLine "All done... Press any key to exit..."
$x = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")
