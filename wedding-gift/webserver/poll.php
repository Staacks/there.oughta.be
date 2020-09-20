<?php

if (isset($_GET["ssid"]) && isset($_GET["ip"]) && isset($_GET["images"]) && isset($_GET["storageused"]) && isset($_GET["storagetotal"])) {
    file_put_contents("status.txt", "SSID: ".$_GET["ssid"]."\nIP: ".$_GET["ip"]."\nImages: ".$_GET["images"]."\nStorage used: ".$_GET["storageused"]."/".$_GET["storagetotal"]);
} else 
    file_put_contents("status.txt", $_SERVER[REQUEST_URI]);

$n = 0;
while (file_exists("content/$n.bin"))
    $n++;

if (isset($_GET["get"]))
    die(base64_encode(file_get_contents("content/".(intval($_GET["get"])).".bin")));
else
    die(strval($n));

?>
