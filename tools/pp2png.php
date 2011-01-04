<?PHP

// pp2png.php - (c) 2008 0xtob
//
// Reads Pocket Physics Sketches and saves the embedded screenshot in png format.

function pp2png($file)
{
	if (!($fp = fopen($file, "r"))) {
		die("could not open XML input");
	}

	$data = fread($fp, 102400); // Max file size: 100KB

	$startpos = stripos  ( $data  , "<image>" ) + 7;
	$endpos = stripos  ( $data  , "</image>" );

	$b64img = substr  ( $data  , $startpos  , $endpos - $startpos);

	$rgb15img = base64_decode($b64img);

	$img = imagecreatetruecolor( 64 , 48 );

	for($i=0; $i<strlen($rgb15img)/2; $i++)
	{
		$n1 = ord($rgb15img[2*$i]);
		$n2 = ord($rgb15img[2*$i+1]);
		$rgb15col = $n1 + 256*$n2;
		$r = floor($rgb15col % 32) * 8;
		$rgb15col = floor($rgb15col / 32);
		$g = floor($rgb15col % 32) * 8;
		$rgb15col = floor($rgb15col / 32);
		$b = floor($rgb15col % 32) * 8;
	
		$col = imagecolorallocate( $img , $r , $g , $b );
		$x = $i % 64;
		$y = floor($i / 64);
		imagesetpixel( $img , $x , $y , $col );
	}

	imagepng($img, "$file.png");
	imagedestroy($img);
}

?>

