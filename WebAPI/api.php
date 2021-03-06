<?php 
  // ------------------ api.php ---------------------

  // Start the PHP session
  session_start();

  // Require https
  if ($_SERVER['HTTPS'] != "on") {

    echo '{"ret":"0"}';
    exit;

  }

  if (isset($_POST["m"])) {

    $res = array();
    $res["ret"] = 1;
    echo json_encode($res);

  } else {

    echo '{"ret":"0"}';

  }

?>
