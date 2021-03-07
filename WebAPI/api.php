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
$versionDB = "01.00.00";

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
      "CREATE TABLE Version (" .
      "  Ref INTEGER PRIMARY KEY," .
      "  Label TEXT NOT NULL)",
      "CREATE TABLE Project (" .
      "  Ref INTEGER PRIMARY KEY," .
      "  Label TEXT NOT NULL)",
      "CREATE TABLE Measure (" .
      "  Ref INTEGER PRIMARY KEY," .
      "  RefProject INTEGER NOT NULL," .
      "  DateMeasure DATETIME NOT NULL)",
      "CREATE TABLE Value (" .
      "  Ref INTEGER PRIMARY KEY," .
      "  RefMeasure INTEGER NOT NULL," .
      "  RefMetric INTEGER NOT NULL," .
      "  Value TEXT NOT NULL)",
      "CREATE TABLE Metric (" .
      "  Ref INTEGER PRIMARY KEY," .
      "  RefProject INTEGER NOT NULL," .
      "  Label TEXT NOT NULL," .
      "  DefaultValue TEXT NOT NULL," .
      "  Unit TEXT NOT NULL)"];
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

// Get the version of the database
function GetVersion($db) {

  $res = array();

  try {

      $rows = $db->query("SELECT Label FROM Version LIMIT 1");
      if ($rows === false) {
        throw new Exception("query() failed");
      }
      
      $res["version"] = ($rows->fetchArray())["Label"];
      $res["ret"] = 0;

  } catch (Exception $e) {

      $res["ret"] = 1;
      $res["errMsg"] = "line " . $e->getLine() . ": " . $e->getMessage();

  }

  return $res;

}

// Upgrade the database to the 
function UpgradeDB($db, $versionDB) {

  // Get the version of the database
  $rows = $db->query("SELECT Label FROM Version LIMIT 1");
  if ($rows === false) {
    throw new Exception("query() failed");
  }
  $version = ($rows->fetchArray())["Label"];

  // If the database version is newer than the version of the API
  if ($version > $versionDB) {

    // The database can't be used
    throw new Exception("Database version (" . $version .
      ") newer than API version (" . $versionDB . ")");

  // If the database version is older than the version of the API
  } else if ($version < $versionDB) {

    // Upgrade the database
    // placeholder...

  }
}

// Add a new project
function AddProject($db, $label) {

  $res = array();

  try {

      // Add the project in the database
      $cmd = 'INSERT INTO Project(Label) VALUES ("' . $label . '")';
      $success = $db->exec($cmd);

      if ($success == false) {
        throw new Exception("exec() failed for " . $cmd);
      }

      $res["refProject"] = $db->lastInsertRowID();
      $res["ret"] = 0;

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

  // Automatically upgrade the database if necessary
  UpgradeDB($db, $versionDB);

  // If an action has been requested
  if (isset($_POST["action"])) {

    if ($_POST["action"] == "version") {

      $res = GetVersion($db);
      echo json_encode($res);

    } else if ($_POST["action"] == "add_project" and 
               isset($_POST["label"])) {

      $res = AddProject($db, $_POST["label"]);
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
