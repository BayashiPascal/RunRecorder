<?php 
// ------------------ api.php ---------------------

// Start the PHP session
session_start();

ini_set('display_errors', 1);
ini_set('display_startup_errors', 1);
error_reporting(E_ALL);

// Path to the database
$pathDB = "./runrecorder.db";

// Version of the database
$versionDB = "1.0";

// Require https
if (!isset($_SERVER['HTTPS'])) {

  echo '{"ret":"0"}';
  exit;

}

// Create the database
function CreateDatabase($path, $version) {

  try {

    // Create and open the database
    $db = new SQLite3($path);

    // Create the database tables
    $cmds = [
      "CREATE TABLE IF NOT EXISTS Version (" .
      "  Ref INTEGER PRIMARY KEY," .
      "  Label TEXT NOT NULL)"];
    foreach ($cmds as $cmd) {

      $success = $db->exec($cmd);

      if ($success == false) {
        throw new Exception("exec() failed for " . $cmd);
      }

    }

    // Set the version
    $success = 
      $db->exec(
        "INSERT INTO Version (Ref, Label) " .
        "VALUES (NULL, '" . $version . "')");
    if ($success == false) {
      throw new Exception("exec() failed for " . $cmd);
    }

    // Return the database connection
    return $db;

  } catch (Exception $e) {

    // Rethrow the exception it will be managed in the main block
    throw($e);

  }

}

// Add a new project
function AddProject($db, $label) {

  $res = array();

  try {


  } catch (Exception $e) {

      $res["ret"] = 1;
      $res["errMsg"] = "line " . $e->getLine() . ": " . $e->getMessage();

  }

  return $res;

}

// Main block
try {

  // Try to open the database without creating it
  try {

    $db = new SQLite3($pathDB, SQLITE3_OPEN_READWRITE);

  // If we couldn't open it, it means it doesn't exist yet
  } catch (Exception $e) {

    // Create the database
    $db = CreateDatabase($pathDB, $versionDB);

  }

  // If an action has been requested
  if (isset($_POST["action"])) {

    if ($_POST["action"] == "add_project" and isset($_POST["label"])) {

      $res = AddProject($db, $_POST["label"]);
      $res["ret"] = 0;
      echo json_encode($res);

    } else {

      echo '{"ret":"1","errMsg":"Invalid action"}';

    }

  } else {

    echo '{"ret":"0"}';

  }

  // Close the database connection
  $db->close();

} catch (Exception $e) {

    $res["ret"] = 1;
    $res["errMsg"] = "line " . $e->getLine() . ": " . $e->getMessage();

}

?>
