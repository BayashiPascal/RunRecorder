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
      "  Label TEXT UNIQUE NOT NULL)",
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
      "  DefaultValue TEXT NOT NULL)"];
    foreach ($cmds as $cmd) {

      $success = $db->exec($cmd);

      if ($success === false) {
        throw new Exception("exec() failed for " . $cmd);
      }

    }

    // Set the version
    $success = 
      $db->exec(
        "INSERT INTO Version (Ref, Label) " .
        "VALUES (NULL, '" . $version . "')");
    if ($success === false) {
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
    $res["ret"] = "0";

  } catch (Exception $e) {

    $res["ret"] = "1";
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

    // Check the label
    if (preg_match('/^[a-zA-Z][a-zA-Z0-9_]*$/', $label) == false) {
      throw new Exception("The label " . $label. " is invalid.");
    }

    // Add the project in the database
    $cmd = 'INSERT INTO Project(Label) VALUES ("' . $label . '")';
    $success = $db->exec($cmd);

    if ($success === false) {
      throw new Exception("exec() failed for " . $cmd);
    }

    $res["refProject"] = "" . $db->lastInsertRowID();
    $res["ret"] = "0";

  } catch (Exception $e) {

    $res["ret"] = "1";
    $res["errMsg"] = "line " . $e->getLine() . ": " . $e->getMessage();

  }

  return $res;

}

// Get the list of projects
function GetProjects($db) {

  $res = array();

  try {

    // Get the projects in the database
    $rows = $db->query('SELECT Ref, Label FROM Project');
    if ($rows === false) {
      throw new Exception("query() failed");
    }
    $res["projects"] = [];
    while ($row = $rows->fetchArray()) {

      $res["projects"][$row["Ref"]] = $row["Label"];

    }

    $res["ret"] = "0";

  } catch (Exception $e) {

    $res["ret"] = "1";
    $res["errMsg"] = "line " . $e->getLine() . ": " . $e->getMessage();

  }

  return $res;

}

// Add a new metric to a project
function AddMetric($db, $project, $label, $default) {

  $res = array();

  try {

    // Check the project reference
    $rows = $db->query('SELECT COUNT(*) as nb FROM Project WHERE Ref = ' .
                       $project);
    if ($rows === false) {
      throw new Exception("query() failed");
    }
    if (($rows->fetchArray())["nb"] != 1) {
      throw new Exception("The project's reference is invalid.");
    }

    // Check the label
    if (preg_match('/^[a-zA-Z][a-zA-Z0-9_]*$/', $label) == false) {
      throw new Exception("The label " . $label. " is invalid.");
    }
    $rows = $db->query('SELECT COUNT(*) as nb FROM Metric WHERE Label = "' .
                       $label . '" AND RefProject = ' . $project);
    if ($rows === false) {
      throw new Exception("query() failed");
    }
    if (($rows->fetchArray())["nb"] != 0) {
      throw new Exception(
        "Another metric with same label already exists for this project.");
    }

    // Check the default value
    if (strlen($default) == 0 or strpos($default, '"') !== false) {
      throw new Exception("The default value is invalid.");
    }

    // Add the metric in the database
    $cmd = 'INSERT INTO Metric(RefProject, Label, DefaultValue) ' .
           'VALUES (' . $project . ', "' . $label . '", "' .
           $default . '")';
    $success = $db->exec($cmd);

    if ($success === false) {
      throw new Exception("exec() failed for " . $cmd);
    }

    $res["ret"] = "0";

  } catch (Exception $e) {

      $res["ret"] = "1";
      $res["errMsg"] = "line " . $e->getLine() . ": " . $e->getMessage();

  }

  return $res;

}

// Get the list of metrics for a project
function GetMetrics($db, $project) {

  $res = array();

  try {

    // Get the metrics for the project
    $rows = $db->query(
      'SELECT Ref, Label FROM Metric WHERE RefProject = ' . $project);
    if ($rows === false) {
      throw new Exception("query() failed");
    }
    $res["metrics"] = [];
    while ($row = $rows->fetchArray()) {

      $res["metrics"][$row["Ref"]] = $row["Label"];

    }

    $res["ret"] = "0";

  } catch (Exception $e) {

    $res["ret"] = "1";
    $res["errMsg"] = "line " . $e->getLine() . ": " . $e->getMessage();

  }

  return $res;

}

// Add a new measure in a project
function AddMeasure($db, $project, $values) {

  $res = array();

  try {

    // Get the date of the record
    $date = date("Y-m-d H:m:s");

    // Add the measure in the database
    $cmd = 'INSERT INTO Measure(RefProject, DateMeasure) VALUES (' . 
           $project . ', "' . $date . '")';
    $success = $db->exec($cmd);
    if ($success === false) {
      throw new Exception("exec() failed for " . $cmd);
    }

    $refMeasure = $db->lastInsertRowID();

    // Loop on the metrics in argument
    foreach ($values as $metric => $value) {

      // Get the reference of the metric
      $rows = $db->query('SELECT Ref FROM Metric WHERE Label = "' .
                         $metric . '" AND RefProject = ' . $project);
      if ($rows === false) {
        throw new Exception("query() failed");
      }

      // If this metric exists
      $row = $rows->fetchArray();
      if ($row !== false) {

        // Add the value
        $cmd = 'INSERT INTO Value(RefMeasure, RefMetric, Value) VALUES (' .
               $refMeasure . ', ' . $row["Ref"] . ', "' . $value . '")';
        $success = $db->exec($cmd);
        if ($success === false) {
          throw new Exception("exec() failed for " . $cmd);
        }

      }
   
    }

    $res["ret"] = "0";

  } catch (Exception $e) {

    $res["ret"] = "1";
    $res["errMsg"] = "line " . $e->getLine() . ": " . $e->getMessage();

  }

  return $res;

}

// Get the list measures for a project
function GetMeasures($db, $project) {

  $res = array();
  $csv = "";

  try {

    // Get the metrics for the project
    $rows = $db->query(
      'SELECT Ref, Label, DefaultValue FROM Metric WHERE RefProject = ' .
      $project . ' ORDER BY Label');
    if ($rows === false) {
      throw new Exception("query() failed");
    }
    $labels = array();
    $refs = array();
    $defs = array();
    while ($row = $rows->fetchArray()) {

      array_push($labels, $row["Label"]);
      array_push($refs, $row["Ref"]);
      array_push($defs, $row["DefaultValue"]);

    }
    $csv = implode('&', $labels) . "\xA";

    // Get the measures for the project
    $rows = $db->query(
      'SELECT Ref FROM Measure WHERE RefProject = ' .
      $project . ' ORDER BY DateMeasure');
    if ($rows === false) {
      throw new Exception("query() failed");
    }
    while ($row = $rows->fetchArray()) {

      // Loop on the metrics
      $vals = array();
      foreach ($refs as $iMetric => $refMetric) {

        // Get the value of this measure for this metric
        $rowsVal = $db->query(
          'SELECT Value FROM Value WHERE RefMeasure = ' .
          $row["Ref"] . ' AND RefMetric = ' . $refMetric);
        if ($rows === false) {
          throw new Exception("query() failed");
        }

        // If there is a value for this metric
        $rowVal = $rowsVal->fetchArray();
        if ($row !== false) {

          $val = $rowVal["Value"];

        // Else, there is no value for this metric
        } else {

          // Use the default value instead
          $val = $defs[$iMetric];

        }

        array_push($vals, $val);

      }

      $csv .= implode('&', $vals) . "\xA";

    }

    $res = $csv;

  } catch (Exception $e) {

    $res["ret"] = "1";
    $res["errMsg"] = "line " . $e->getLine() . ": " . $e->getMessage();
    $res = json_encode($res);

  }

  return $res;

}

// Flush a project
function FlushProject($db, $project) {

  $res = array();

  try {

    // Delete the values of the project
    $cmd = 'DELETE FROM Value WHERE RefMeasure IN (SELECT Ref FROM Measure WHERE RefProject = ' . $project . ')';
    $success = $db->exec($cmd);
    if ($success === false) {
      throw new Exception("exec() failed for " . $cmd);
    }

    // Delete the measures of the project
    $cmd = 'DELETE FROM Measure WHERE RefProject = ' . $project;
    $success = $db->exec($cmd);
    if ($success === false) {
      throw new Exception("exec() failed for " . $cmd);
    }

    // Delete the metrics of the project
    $cmd = 'DELETE FROM Metric WHERE RefProject = ' . $project;
    $success = $db->exec($cmd);
    if ($success === false) {
      throw new Exception("exec() failed for " . $cmd);
    }

    // Delete the project
    $cmd = 'DELETE FROM Project WHERE Ref = ' . $project;
    $success = $db->exec($cmd);
    if ($success === false) {
      throw new Exception("exec() failed for " . $cmd);
    }

    $res["ret"] = "0";

  } catch (Exception $e) {

    $res["ret"] = "1";
    $res["errMsg"] = "line " . $e->getLine() . ": " . $e->getMessage();

  }

  return $res;

}

// -------------------------------- Main block --------------------------

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

    } else if ($_POST["action"] == "projects") {

      $res = GetProjects($db);
      echo json_encode($res);

    } else if ($_POST["action"] == "add_metric" and 
               isset($_POST["project"]) and
               isset($_POST["label"]) and
               isset($_POST["default"])) {

      $res = AddMetric($db, $_POST["project"], $_POST["label"],
                       $_POST["default"]);
      echo json_encode($res);

    } else if ($_POST["action"] == "metrics" and 
               isset($_POST["project"])) {

      $res = GetMetrics($db, $_POST["project"]);
      echo json_encode($res);

    } else if ($_POST["action"] == "add_measure" and 
               isset($_POST["project"])) {

      $res = AddMeasure($db, $_POST["project"], $_POST);
      echo json_encode($res);

    } else if ($_POST["action"] == "measures" and 
               isset($_POST["project"])) {

      $res = GetMeasures($db, $_POST["project"]);
      echo $res;

    } else if ($_POST["action"] == "flush" and 
               isset($_POST["project"])) {

      $res = FlushProject($db, $_POST["project"]);
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

    $res["ret"] = "1";
    $res["errMsg"] = "line " . $e->getLine() . ": " . $e->getMessage();

}

?>
