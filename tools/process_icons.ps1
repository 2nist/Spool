[Reflection.Assembly]::LoadWithPartialName('System.Drawing') > $null
$src = Join-Path (Get-Location) 'assets\icons'
Get-ChildItem -Path $src -Filter '*.png' | ForEach-Object {
    $in = $_.FullName
    try {
        $bmp = [System.Drawing.Image]::FromFile($in)
        $dest = New-Object System.Drawing.Bitmap 22,22
        $g = [System.Drawing.Graphics]::FromImage($dest)
        $g.Clear([System.Drawing.Color]::Transparent)
        $g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
        $g.DrawImage($bmp, 0, 0, 22, 22)
        $g.Dispose()
        for ($x = 0; $x -lt 22; $x++) {
            for ($y = 0; $y -lt 22; $y++) {
                $c = $dest.GetPixel($x, $y)
                if ($c.A -ne 0) {
                    $dest.SetPixel($x, $y, [System.Drawing.Color]::FromArgb($c.A, 255, 255, 255))
                }
            }
        }
        $dest.Save($in, [System.Drawing.Imaging.ImageFormat]::Png)
        $dest.Dispose()
        $bmp.Dispose()
        Write-Host "Processed: $in"
    } catch {
        Write-Host "Failed: $in -> $_"
    }
}
