<?php
$n = 0;
while (file_exists("content/$n.bin"))
    $n++;

if (isset($_POST["deleteid"])) {
    unlink("content/".((int)$_POST["deleteid"]).".bin");
    $n--;
    if ($n > 0 && $n != (int)$_POST["deleteid"])
        rename("content/$n.bin", "content/".((int)$_POST["deleteid"]).".bin");
}
?>

<?php
echo "Images on server: ".$n."<br />";
echo "Last poll: ".date ("d.m.Y H:i:s", filemtime("status.txt"))."<br />";
echo str_replace("\n", "<br />", file_get_contents("status.txt"))
?>

<hr>

<?php
for ($i = 0; $i < $n; $i++) {
?>
    <div style="margin: 1em; max-width: 100vw;">
      <img style="border: 1px solid black; max-width: 80vw" src="preview.php?id=<?php echo $i;?>" />
      <form action="status.php" method="post">
        <input type="hidden" name="deleteid" value="<?php echo $i;?>" />
        <button>Delete</button>
      </form>
    </div>
<?php
}
?>
