<?php

  $id = (int)$_GET["id"];

  $handle = fopen("content/$id.bin", "rb");
  $data = unpack("C*", fread($handle, 800*480));
  fclose($handle);

  $img = @imagecreate(800, 480);

  $w = imagecolorallocate($img, 255, 255, 255);
  $b = imagecolorallocate($img, 0, 0, 0);

  $index = 1;
  $end = count($data);

  $line = 0;
  while ($index < $end) {
    $repetitions = $data[$index++];
    $x = 0;
    if ($repetitions == 0) { //The next 100 bytes simply encode pixels directly
      for ($j = 0; $j < 100; $j++) {
        if ($index >= $end)
          break;
        $pixelData = $data[$index++];
        for ($mask = 7; $mask >= 0; $mask--) {
          if ($pixelData & (1 << $mask))
            imagesetpixel($img, $x, $line, $b);
          else
            imagesetpixel($img, $x, $line, $w);
          $x++;
        }
      }
    } else { //The next [repetitions] bytes encode number of identical pixels, starting with white (which of course may be zero)
      $white = true;
      for ($j = 0; $j < $repetitions; $j++) {
        if ($index >= $end)
          break;
        $pixelCount = $data[$index++];
        if ($white) { // White
          for ($xi = 0; $xi < $pixelCount; $xi++) {
            imagesetpixel($img, $x, $line, $w);
            $x++;
          }
        } else { // Black
          for ($xi = 0; $xi < $pixelCount; $xi++) {
            imagesetpixel($img, $x, $line, $b);
            $x++;
          } 
        }
        if ($pixelCount < 255)
          $white = !$white;
      }
    }
    $line++;
  }

  header("Content-type: image/png");

  imagepng($img);
  imagedestroy($img);

?>
