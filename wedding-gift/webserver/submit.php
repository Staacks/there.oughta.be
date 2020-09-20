<?php

$n = 0;
while (file_exists("content/".$n.".bin"))
    $n++;

$data = file_get_contents("php://input");
file_put_contents("content/".$n.".bin", $data);

?>
