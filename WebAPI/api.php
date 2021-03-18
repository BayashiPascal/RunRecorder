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
      "CREATE TABLE _Version (" .
      "  Ref INTEGER PRIMARY KEY," .
      "  Label TEXT NOT NULL)",
      "CREATE TABLE _Project (" .
      "  Ref INTEGER PRIMARY KEY," .
      "  Label TEXT UNIQUE NOT NULL)",
      "CREATE TABLE _Measure (" .
      "  Ref INTEGER PRIMARY KEY," .
      "  RefProject INTEGER NOT NULL," .
      "  DateMeasure DATETIME NOT NULL)",
      "CREATE TABLE _Value (" .
      "  Ref INTEGER PRIMARY KEY," .
      "  RefMeasure INTEGER NOT NULL," .
      "  RefMetric INTEGER NOT NULL," .
      "  Value TEXT NOT NULL)",
      "CREATE TABLE _Metric (" .
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
        "INSERT INTO _Version (Ref, Label) " .
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

    $rows = $db->query("SELECT Label FROM _Version LIMIT 1");
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
  $rows = $db->query("SELECT Label FROM _Version LIMIT 1");
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

    // If the project doesn't already exists
    $rows = $db->query(
      'SELECT COUNT(*) as nb FROM _Project WHERE Label = "' . $label . '"');
    if ($rows === false) {
      throw new Exception("query() failed");
    }
    if (($rows->fetchArray())["nb"] == 0) {

      // Add the project in the database
      $cmd = 'INSERT INTO _Project(Label) VALUES ("' . $label . '")';
      $success = $db->exec($cmd);

      if ($success === false) {
        throw new Exception("exec() failed for " . $cmd);
      }

    }

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
    $rows = $db->query('SELECT Ref, Label FROM _Project');
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

// Get the reference of a project from its name
function GetRefProject($db, $project) {

  // Get the project reference
  $rows = $db->query(
    'SELECT Ref FROM _Project WHERE Label = "' .
    $project . '"');
  if ($rows === false) {
    throw new Exception("query() failed");
  }
  $row = $rows->fetchArray();
  if ($row === false) {
    throw new Exception("The project's name is invalid.");
  }
  $refProject = $row["Ref"];

  // Return the reference
  return $refProject;

}

// Function to update the view for a project
function UpdateViewProject($db, $project) {

  // Ensure the view doesn't exist
  $cmd = "DROP VIEW IF EXISTS " . $project;
  $success = $db->exec($cmd);
  if ($success === false) {
    throw new Exception("exec() failed for " . $cmd);
  }

  // Get the project reference
  $refProject = GetRefProject($db, $project);

  // Get the metrics for the project
  $cmd = 'SELECT Ref, Label FROM _Metric WHERE RefProject = ' .
    $refProject . ' ORDER BY Label';
  $rows = $db->query($cmd);
  if ($rows === false) {
    throw new Exception("query(" . $cmd . ") failed");
  }
  $labels = array();
  $refs = array();
  $defs = array();
  while ($row = $rows->fetchArray()) {

    array_push($labels, $row["Label"]);
    array_push($refs, $row["Ref"]);

  }

  // Create the command for the view
  $cmd = "CREATE VIEW " . $project . " (Ref";
  foreach($labels as $label) {

    $cmd .= "," . $label;

  }
  $cmd .= ") AS SELECT _Measure.Ref ";
  foreach($refs as $ref) {

    $cmd .= ",IFNULL((SELECT Value FROM _Value ";
    $cmd .= "WHERE RefMeasure=_Measure.Ref AND RefMetric=";
    $cmd .= $ref;
    $cmd .= "),(SELECT DefaultValue FROM _Metric WHERE Ref=";
    $cmd .= $ref . ")) ";

  }
  $cmd .= "FROM _Measure ORDER BY _Measure.DateMeasure, _Measure.Ref";

  // Create the view
  $success = $db->exec($cmd);
  if ($success === false) {
    throw new Exception("exec() failed for " . $cmd);
  }

}

// Add a new metric to a project
function AddMetric($db, $project, $label, $default) {

  $res = array();

  try {

    // Get the project reference
    $refProject = GetRefProject($db, $project);

    // Check the label
    if (preg_match('/^[a-zA-Z][a-zA-Z0-9_]*$/', $label) == false) {
      throw new Exception("The label " . $label. " is invalid.");
    }

    // If the metric doesn't already exists
    $rows = $db->query('SELECT COUNT(*) as nb FROM _Metric WHERE Label = "' .
                       $label . '" AND RefProject = ' . $refProject);
    if ($rows === false) {
      throw new Exception("query() failed");
    }
    if (($rows->fetchArray())["nb"] == 0) {

      // Check the default value
      if (strlen($default) == 0 or strpos($default, '"') !== false) {
        throw new Exception("The default value is invalid.");
      }

      // Add the metric in the database
      $cmd = 'INSERT INTO _Metric(RefProject, Label, DefaultValue) ' .
             'VALUES (' . $refProject . ', "' . $label . '", "' .
             $default . '")';
      $success = $db->exec($cmd);

      if ($success === false) {
        throw new Exception("exec() failed for " . $cmd);
      }

    }

    // Update the view for the project
    UpdateViewProject($db, $project);

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
      'SELECT _Metric.Ref, _Metric.Label FROM _Metric, _Project ' .
      'WHERE _Metric.RefProject = _Project.Ref AND ' . 
      '_Project.Label = "' . $project . '"');
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

    // Get the project reference
    $refProject = GetRefProject($db, $project);

    // Get the date of the record
    $date = date("Y-m-d H:m:s");

    // Add the measure in the database
    $cmd = 'INSERT INTO _Measure(RefProject, DateMeasure) VALUES (' . 
           $refProject . ', "' . $date . '")';
    $success = $db->exec($cmd);
    if ($success === false) {
      throw new Exception("exec() failed for " . $cmd);
    }

    $refMeasure = $db->lastInsertRowID();

    // Declare a variable to memorise an eventual failure
    // The policy here is to try to save has much data has possible
    // even if some fails, inform the user and let him/her take
    // appropriate action
    $hasFailed = false;

    // Loop on the metrics in argument
    foreach ($values as $metric => $value) {

      // Get the reference of the metric
      $rows = $db->query('SELECT Ref FROM _Metric WHERE Label = "' .
                         $metric . '" AND RefProject = ' . $refProject);
      if ($rows === false) {
        throw new Exception("query() failed");
      }

      // If this metric exists
      $row = $rows->fetchArray();
      if ($row !== false) {

        // Add the value
        $cmd = 'INSERT INTO _Value(RefMeasure, RefMetric, Value) VALUES (' .
               $refMeasure . ', ' . $row["Ref"] . ', "' . $value . '")';
        $success = $db->exec($cmd);
        if ($success === false) {
          $hasFailed = true;
        }

      }
   
    }

    $res["refMeasure"] = "" . $refMeasure;

    if ($hasFailed == true) {
      throw new Exception("exec() failed for INSERT INTO _Value");
    }

    $res["ret"] = "0";

  } catch (Exception $e) {

    $res["ret"] = "1";
    $res["errMsg"] = "line " . $e->getLine() . ": " . $e->getMessage();

  }

  return $res;

}

// Delete a measure
function DeleteMeasure($db, $measure) {

  $res = array();

  try {

    // Delete the values of the measure in the database
    $cmd = 'DELETE FROM _Value WHERE RefMeasure = ' . $measure;
    $success = $db->exec($cmd);
    if ($success === false) {
      throw new Exception("exec() failed for " . $cmd);
    }

    // Delete the measure in the database
    $cmd = 'DELETE FROM _Measure WHERE Ref = ' . $measure;
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

// Get the list measures for a project as CSV
function GetMeasuresAsCSV($db, $project, $sep) {

  $res = array();

  try {

    $measures = GetMeasures($db, $project, 0);
    if ($measures["ret"] != "0") {
      return $measures;
    } 
    $csv = implode($sep, $measures["labels"]) . "\xA";
    foreach ($measures["values"] as $measure) {
      $csv .= implode('&', $measure) . "\xA";
    }
    $res = $csv;

  } catch (Exception $e) {

    $res["ret"] = "1";
    $res["errMsg"] = "line " . $e->getLine() . ": " . $e->getMessage();
    $res = json_encode($res);

  }

  return $res;

}

// Get the list measures for a project
function GetMeasures($db, $project, $nbMeasure) {

  $res = array();

  try {

    // Get the project reference
    $refProject = GetRefProject($db, $project);

    // Get the metrics for the project
    $cmd = 'SELECT Label FROM _Metric WHERE RefProject = ' .
      $refProject . ' ORDER BY Label';
    $rows = $db->query($cmd);
    if ($rows === false) {
      throw new Exception("query(" . $cmd . ") failed");
    }
    $res["labels"] = array();
    while ($row = $rows->fetchArray()) {
      array_push($res["labels"], $row["Label"]);
    }

    // Get the measures for the project
    $cmd = 'SELECT Ref';
    // Loop on the metrics
    foreach ($res["labels"] as $label) {
      $cmd .= ', ' . $label;
    }
    $cmd .= ' FROM ' . $project;

    // If there is a limit on the number of returned measures
    if ($nbMeasure > 0) {
      $cmd .= ' ORDER BY Ref DESC LIMIT ' . $nbMeasure;
    // Else, there is no limit on the number of returned measures
    } else {
      $cmd .= ' ORDER BY Ref ASC';
    }

    $rows = $db->query($cmd);
    if ($rows === false) {
      throw new Exception("query(" . $cmd . ") failed");
    }
    $res["values"] = array();
    while ($row = $rows->fetchArray()) {

      // Loop on the metrics
      $values = [];
      foreach ($res["labels"] as $label) {
        array_push($values, $row[$label]);
      }
      array_push($res["values"], $values);

    }

    $res["ret"] = "0";

  } catch (Exception $e) {

    $res["ret"] = "1";
    $res["errMsg"] = "line " . $e->getLine() . ": " . $e->getMessage();

  }

  return $res;

}

// Flush a project
function FlushProject($db, $project) {

  $res = array();

  try {

    // Get the project reference
    $refProject = GetRefProject($db, $project);

    // Delete the values of the project
    $cmd = 'DELETE FROM _Value WHERE RefMeasure IN ' .
           '(SELECT Ref FROM _Measure WHERE RefProject = ' .
           $refProject . ')';
    $success = $db->exec($cmd);
    if ($success === false) {
      throw new Exception("exec() failed for " . $cmd);
    }

    // Delete the measures of the project
    $cmd = 'DELETE FROM _Measure WHERE RefProject = ' . $refProject;
    $success = $db->exec($cmd);
    if ($success === false) {
      throw new Exception("exec() failed for " . $cmd);
    }

    // Delete the metrics of the project
    $cmd = 'DELETE FROM _Metric WHERE RefProject = ' . $refProject;
    $success = $db->exec($cmd);
    if ($success === false) {
      throw new Exception("exec() failed for " . $cmd);
    }

    // Delete the project
    $cmd = 'DELETE FROM _Project WHERE Ref = ' . $refProject;
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

    } else if ($_POST["action"] == "delete_measure" and 
               isset($_POST["measure"])) {

      $res = DeleteMeasure($db, $_POST["measure"]);
      echo json_encode($res);

    } else if ($_POST["action"] == "measures" and 
               isset($_POST["project"])) {

      // If the user hasn't specified a limit for the number of returned
      // measure, set it by default to 0
      if (!isset($_POST["last"])) $_POST["last"] = 0;
      $res = GetMeasures($db, $_POST["project"], $_POST["last"]);
      echo json_encode($res);

    } else if ($_POST["action"] == "csv" and 
               isset($_POST["project"])) {

      $res = GetMeasuresAsCSV($db, $_POST["project"], '&');
      echo $res;

    } else if ($_POST["action"] == "flush" and 
               isset($_POST["project"])) {

      $res = FlushProject($db, $_POST["project"]);
      echo json_encode($res);

    } else if ($_POST["action"] == "help") {

      echo '{"ret":"0","actions":"version, ' . 
        'add_project&label=..., ' .
        'projects, ' .
        'add_metric&project=...&label=...&default=..., ' .
        'metrics&project=..., ' .
        'add_measure&project=...&...=...&..., ' .
        'delete_measure&measure=..., ' .
        'csv&project=..., ' .
        'flush&project=..."}';

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
