<?php 
// ------------------ api.php ---------------------

// Start the PHP session
session_start();

// Switch the display of errors
//ini_set('display_errors', 1);
//ini_set('display_startup_errors', 1);
//error_reporting(E_ALL);
error_reporting(E_NONE);

// Path to the SQLite3 database
$pathDB = "./runrecorder.db";

// Version of the database
$versionDB = "01.00.00";

// Create the database
// Inputs:
//      path: path of the database
//   version: version of the database
// Output:
//   Return the database connection
function CreateDatabase(
  $path,
  $version) {

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

      if ($success === false) throw new Exception("exec() failed for " . $cmd);

    }

    // Set the version
    $success = 
      $db->exec(
        "INSERT INTO _Version (Ref, Label) " .
        "VALUES (NULL, '" . $version . "')");
    if ($success === false) throw new Exception("exec() failed for " . $cmd);

    // Return the database connection
    return $db;

  } catch (Exception $e) {

    // Rethrow the exception it will be managed in the main block
    throw($e);

  }

}

// Get the version of the database
// Input:
//   db: the database connection
// Output:
//   If successful returns the dictionary {"ret":"0", "version":"..."}.
//   Else, returns the dictionary {"ret":"1", "errMsg":"..."}.
function GetVersion(
  $db) {

  // Initialise the result dictionary
  $res = array();

  try {

    // Get the version
    $rows = $db->query("SELECT Label FROM _Version LIMIT 1");
    if ($rows === false) throw new Exception("query() failed");

    // Update the result dictionary
    $res["version"] = ($rows->fetchArray())["Label"];
    $res["ret"] = "0";

  } catch (Exception $e) {

    $res["ret"] = "1";
    $res["errMsg"] = "line " . $e->getLine() . ": " . $e->getMessage();

  }

  // Return the dictionary
  return $res;

}

// Upgrade the database to a given version
// Input:
//           db: the database connection
//   tgtVersion: the requested version
function UpgradeDB(
  $db,
  $tgtVersion) {

  // Get the version of the database
  $rows = $db->query("SELECT Label FROM _Version LIMIT 1");
  if ($rows === false) throw new Exception("query() failed");
  $version = ($rows->fetchArray())["Label"];

  // If the database version is older than the version of the API
  if ($version < $tgtVersion) {

    // Upgrade the database
    // placeholder...

  }
}

// Add a new project
// Input:
//      db: the database connection
//   label: the name of the project
// Output:
//   If successful returns the dictionary {"ret":"0"}.
//   Else, returns the dictionary {"ret":"1", "errMsg":"..."}.
function AddProject(
  $db,
  $label) {

  // Init the result dictionary
  $res = array();

  try {

    // Check the label
    if (preg_match('/^[a-zA-Z][a-zA-Z0-9_]*$/', $label) == false)
      throw new Exception("The label " . $label. " is invalid.");

    // If the project doesn't already exists
    $rows = $db->query(
      'SELECT COUNT(*) as nb FROM _Project WHERE Label = "' . $label . '"');
    if ($rows === false) throw new Exception("query() failed");
    if (($rows->fetchArray())["nb"] == 0) {

      // Add the project in the database
      $cmd = 'INSERT INTO _Project(Label) VALUES ("' . $label . '")';
      $success = $db->exec($cmd);

      if ($success === false) throw new Exception("exec() failed for " . $cmd);

    }

    // Set the success code in the result dictionary
    $res["ret"] = "0";

  } catch (Exception $e) {

    $res["ret"] = "1";
    $res["errMsg"] = "line " . $e->getLine() . ": " . $e->getMessage();

  }

  // Return the dictionary
  return $res;

}

// Get the list of projects
// Input:
//   db: the database connection
// Output:
//   If successful returns the dictionary {"ret":"0",
//   "projects":["...", "...", ...]}.
//   Else, returns the dictionary {"ret":"1", "errMsg":"..."}.
function GetProjects(
  $db) {

  // Init the result dictionary
  $res = array();

  try {

    // Get the projects in the database
    $rows = $db->query('SELECT Ref, Label FROM _Project');
    if ($rows === false) throw new Exception("query() failed");
    $res["projects"] = [];
    while ($row = $rows->fetchArray())
      $res["projects"][$row["Ref"]] = $row["Label"];

    // Set the success code in the result dictionary
    $res["ret"] = "0";

  } catch (Exception $e) {

    $res["ret"] = "1";
    $res["errMsg"] = "line " . $e->getLine() . ": " . $e->getMessage();

  }

  // Return the dictionary
  return $res;

}

// Get the reference of a project from its name
// Input:
//        db: the database connection
//   project: the project's name
// Output:
//   Returns the project's reference in the database.
function GetRefProject(
  $db,
  $project) {

  // Get the project reference
  $rows = $db->query(
    'SELECT Ref FROM _Project WHERE Label = "' .
    $project . '"');
  if ($rows === false) throw new Exception("query() failed");
  $row = $rows->fetchArray();
  if ($row === false) throw new Exception("The project's name is invalid.");
  $refProject = $row["Ref"];

  // Return the reference
  return $refProject;

}

// Function to update the view for a project
// Input:
//        db: the database connection
//   project: the project's name
function UpdateViewProject(
  $db,
  $project) {

  // Ensure the view doesn't exist
  $cmd = "DROP VIEW IF EXISTS " . $project;
  $success = $db->exec($cmd);
  if ($success === false) throw new Exception("exec() failed for " . $cmd);

  // Get the project reference
  $refProject =
    GetRefProject(
      $db,
      $project);

  // Get the metrics for the project
  $cmd = 'SELECT Ref, Label FROM _Metric WHERE RefProject = ' .
    $refProject . ' ORDER BY Label';
  $rows = $db->query($cmd);
  if ($rows === false) throw new Exception("query(" . $cmd . ") failed");
  $labels = array();
  $refs = array();
  $defs = array();
  while ($row = $rows->fetchArray()) {

    array_push(
      $labels,
      $row["Label"]);
    array_push(
      $refs,
      $row["Ref"]);

  }

  // Create the command for the view
  $cmd = "CREATE VIEW " . $project . " (Ref";
  foreach($labels as $label) $cmd .= "," . $label;
  $cmd .= ") AS SELECT _Measure.Ref ";
  foreach($refs as $ref) {

    $cmd .= ",IFNULL((SELECT Value FROM _Value ";
    $cmd .= "WHERE RefMeasure=_Measure.Ref AND RefMetric=";
    $cmd .= $ref;
    $cmd .= "),(SELECT DefaultValue FROM _Metric WHERE Ref=";
    $cmd .= $ref . ")) ";

  }
  $cmd .= "FROM _Measure WHERE _Measure.RefProject = ";
  $cmd .= $refProject . " ORDER BY _Measure.DateMeasure, _Measure.Ref";

  // Create the view
  $success = $db->exec($cmd);
  if ($success === false) throw new Exception("exec() failed for " . $cmd);

}

// Add a new metric to a project
// Input:
//        db: the database connection
//   project: the project's name
//     label: the metrics's label
//   default: the metric's default value
// Output:
//   If successful returns the dictionary {"ret":"0"}.
//   Else, returns the dictionary {"ret":"1", "errMsg":"..."}.
function AddMetric(
  $db,
  $project,
  $label,
  $default) {

  // Init the result dictionary
  $res = array();

  try {

    // Get the project reference
    $refProject =
      GetRefProject(
        $db,
        $project);

    // Check the label
    if (
      preg_match('/^[a-zA-Z][a-zA-Z0-9_]*$/', $label) == false or
      $label == "project" or
      $label == "action")
      throw new Exception("The label " . $label. " is invalid.");

    // Check the default value
    if (preg_match('/^[^"=&]+$/', $default) == false)
      throw new Exception("The default value " . $default . " is invalid.");

    // If the metric doesn't already exists
    $rows = $db->query('SELECT COUNT(*) as nb FROM _Metric WHERE Label = "' .
                       $label . '" AND RefProject = ' . $refProject);
    if ($rows === false) throw new Exception("query() failed");
    if (($rows->fetchArray())["nb"] == 0) {

      // Check the default value
      if (strlen($default) == 0 or strpos($default, '"') !== false)
        throw new Exception("The default value is invalid.");

      // Add the metric in the database
      $cmd = 'INSERT INTO _Metric(RefProject, Label, DefaultValue) ' .
             'VALUES (' . $refProject . ', "' . $label . '", "' .
             $default . '")';
      $success = $db->exec($cmd);
      if ($success === false) throw new Exception("exec() failed for " . $cmd);

    }

    // Update the view for the project
    UpdateViewProject($db, $project);

    // Set the success code in the result dictionary
    $res["ret"] = "0";

  } catch (Exception $e) {

      $res["ret"] = "1";
      $res["errMsg"] = "line " . $e->getLine() . ": " . $e->getMessage();

  }

  // Return the dictionary
  return $res;

}

// Get the list of metrics for a project
// Input:
//        db: the database connection
//   project: the project's name
// Output:
//   If successful returns the dictionary {"ret":"0",
//   "metrics":["Ref1":["Label":"Label1", "DefaultValue":"Value1"],
//   "Ref2":["Label":"Label2", "DefaultValue":"Value2"],...]}.
//   Else, returns the dictionary {"ret":"1", "errMsg":"..."}.
function GetMetrics(
  $db,
  $project) {

  // Init the result dictionary
  $res = array();

  try {

    // Get the metrics for the project
    $rows = $db->query(
      'SELECT _Metric.Ref, _Metric.Label, _Metric.DefaultValue ' .
      'FROM _Metric, _Project ' .
      'WHERE _Metric.RefProject = _Project.Ref AND ' . 
      '_Project.Label = "' . $project . '"');
    if ($rows === false) throw new Exception("query() failed");
    $res["metrics"] = [];
    while ($row = $rows->fetchArray()) {
      $res["metrics"][$row["Ref"]] = [];
      $res["metrics"][$row["Ref"]]["Label"] = $row["Label"];
      $res["metrics"][$row["Ref"]]["DefaultValue"] = $row["DefaultValue"];
    }

    // Set the success code in the result dictionary
    $res["ret"] = "0";

  } catch (Exception $e) {

    $res["ret"] = "1";
    $res["errMsg"] = "line " . $e->getLine() . ": " . $e->getMessage();

  }

  // Return the dictionary
  return $res;

}

// Add a new measure in a project
// Input:
//        db: the database connection
//   project: the project's name
//    values: the values of the measure as a dictionary
//            {"metricA":"valueA", "metricB":"valueB",...}
// Output:
//   If successful returns the dictionary {"ret":"0", "refMeasure":"..."}.
//   Else, returns the dictionary {"ret":"1", "errMsg":"..."}.
function AddMeasure(
  $db,
  $project,
  $values) {

  $res = array();

  try {

    // Get the project reference
    $refProject =
      GetRefProject(
        $db,
        $project);

    // Get the date of the record
    $date = date("Y-m-d H:m:s");

    // Add the measure in the database
    $cmd = 'INSERT INTO _Measure(RefProject, DateMeasure) VALUES (' . 
           $refProject . ', "' . $date . '")';
    $success = $db->exec($cmd);
    if ($success === false) throw new Exception("exec() failed for " . $cmd);

    // Get the reference of the new measure
    $refMeasure = $db->lastInsertRowID();

    // Declare a variable to memorise an eventual failure
    // The policy here is to try to save has much data has possible
    // even if some fails, inform the user and let him/her take
    // appropriate action
    $hasFailed = false;

    // Loop on the metrics in argument
    foreach ($values as $metric => $value) {

      // If the value is not valid
      $isValidValue =
        preg_match(
          '/^[^"=&]+$/',
          $value);
      if ($isValidValue == false) {

        $hasFailed = true;

      // Else, the value is valid
      } else {

        // Get the reference of the metric
        $rows = $db->query('SELECT Ref FROM _Metric WHERE Label = "' .
                           $metric . '" AND RefProject = ' . $refProject);
        if ($rows === false) throw new Exception("query() failed");

        // If this metric exists
        $row = $rows->fetchArray();
        if ($row !== false) {

          // Add the value
          $cmd = 'INSERT INTO _Value(RefMeasure, RefMetric, Value) VALUES (' .
                 $refMeasure . ', ' . $row["Ref"] . ', "' . $value . '")';
          $success = $db->exec($cmd);
          if ($success === false) $hasFailed = true;

        }

      }
   
    }

    // Memorise the reference of the new measure as a string
    $res["refMeasure"] = "" . $refMeasure;

    if ($hasFailed == true)
      throw new Exception("exec() failed for INSERT INTO _Value");

    // Set the success code in the result dictionary
    $res["ret"] = "0";

  } catch (Exception $e) {

    $res["ret"] = "1";
    $res["errMsg"] = "line " . $e->getLine() . ": " . $e->getMessage();

  }

  // Return the dictionary
  return $res;

}

// Delete a measure
// Input:
//        db: the database connection
//   measure: the reference of the measure
// Output:
//   If successful returns the dictionary {"ret":"0"}.
//   Else, returns the dictionary {"ret":"1", "errMsg":"..."}.
function DeleteMeasure(
  $db,
  $measure) {

  // Init the result dictionary
  $res = array();

  try {

    // Delete the values of the measure in the database
    $cmd = 'DELETE FROM _Value WHERE RefMeasure = ' . $measure;
    $success = $db->exec($cmd);
    if ($success === false) throw new Exception("exec() failed for " . $cmd);

    // Delete the measure in the database
    $cmd = 'DELETE FROM _Measure WHERE Ref = ' . $measure;
    $success = $db->exec($cmd);
    if ($success === false) throw new Exception("exec() failed for " . $cmd);

    // Set the success code in the result dictionary
    $res["ret"] = "0";

  } catch (Exception $e) {

    $res["ret"] = "1";
    $res["errMsg"] = "line " . $e->getLine() . ": " . $e->getMessage();

  }

  // Return the dictionary
  return $res;

}

// Get the list measures for a project as CSV
// Input:
//        db: the database connection
//   project: the project's name
//       sep: the character used as a separator
// Output:
//   If successful returns the data in CSV format as (e.g. sep=&)
//   metricA&metricB&...
//   valueA1&valueB1&...
//   valueA2&valueB2&...
//   ...
//   Else, returns the dictionary {"ret":"1", "errMsg":"..."}.
function GetMeasuresAsCSV(
  $db,
  $project,
  $sep) {

  // Init the result dictionary
  $res = array();

  try {

    // Get all the measures
    $nbMeasure = 0;
    $measures =
      GetMeasures(
        $db,
        $project,
        $nbMeasure);
    if ($measures["ret"] != "0") return $measures;
    
    // Create the first line with the metrics' label
    $csv = implode($sep, $measures["labels"]) . "\xA";

    // Add the measures' values
    foreach ($measures["values"] as $measure)
      $csv .= implode($sep, $measure) . "\xA";

    // Update the result
    $res = $csv;

  } catch (Exception $e) {

    $res["ret"] = "1";
    $res["errMsg"] = "line " . $e->getLine() . ": " . $e->getMessage();
    $res = json_encode($res);

  }

  // Return the result
  return $res;

}

// Get the list of measures for a project
// Input:
//          db: the database connection
//     project: the project's name
//   nbMeasure: maximum number of measures to be returned. If 0, returns
//              all the measure in the order they were added. If >0 returns
//              at maximum the last nbMeasure measures ordered from the
//              most recent to the oldest.
// Output:
//   If successful returns a dictionary {"ret":"0", "labels":["metricA",
//   metricB", ...], "values":[["valueA1", "valueA2"], ["valueB1",
//   "valueB2"], ...]}
//   Else, returns the dictionary {"ret":"1", "errMsg":"..."}.
function GetMeasures(
  $db,
  $project,
  $nbMeasure) {

  // Init the result dictionary
  $res = array();

  try {

    // Get the project reference
    $refProject =
      GetRefProject(
        $db,
        $project);

    // Get the metrics for the project
    $cmd = 'SELECT Label FROM _Metric WHERE RefProject = ' .
      $refProject . ' ORDER BY Label';
    $rows = $db->query($cmd);
    if ($rows === false) throw new Exception("query(" . $cmd . ") failed");

    // Add the metrics' label to the result dictionary
    $res["labels"] = array();
    array_push($res["labels"], "Ref");
    while ($row = $rows->fetchArray())
      array_push($res["labels"], $row["Label"]);

    // Create the command to get the measures for the project
    $cmd = 'SELECT Ref';
    foreach ($res["labels"] as $label) $cmd .= ', ' . $label;
    $cmd .= ' FROM ' . $project;

    // Order the measures according to the number of returned measures
    if ($nbMeasure > 0)
      $cmd .= ' ORDER BY Ref DESC LIMIT ' . $nbMeasure;
    else
      $cmd .= ' ORDER BY Ref ASC';
 
    // Get the measures
    $rows = $db->query($cmd);
    if ($rows === false) throw new Exception("query(" . $cmd . ") failed");
    $res["values"] = array();
    while ($row = $rows->fetchArray()) {

      // Add the values to the result dictionary
      $values = [];
      foreach ($res["labels"] as $label) array_push($values, $row[$label]);
      array_push($res["values"], $values);

    }

    // Set the success code in the result dictionary
    $res["ret"] = "0";

  } catch (Exception $e) {

    $res["ret"] = "1";
    $res["errMsg"] = "line " . $e->getLine() . ": " . $e->getMessage();

  }

  // Return the dictionary
  return $res;

}

// Flush a project
// Input:
//         db: the database connection
//    project: the project's name
// Output:
//   If successful returns a dictionary {"ret":"0"}
//   Else, returns the dictionary {"ret":"1", "errMsg":"..."}.
function FlushProject(
  $db,
  $project) {

  // Init the result dictionary
  $res = array();

  try {

    // Get the project reference
    $refProject =
      GetRefProject(
        $db,
        $project);

    // Delete the values of the project
    $cmd = 'DELETE FROM _Value WHERE RefMeasure IN ' .
           '(SELECT Ref FROM _Measure WHERE RefProject = ' .
           $refProject . ')';
    $success = $db->exec($cmd);
    if ($success === false) throw new Exception("exec() failed for " . $cmd);

    // Delete the measures of the project
    $cmd = 'DELETE FROM _Measure WHERE RefProject = ' . $refProject;
    $success = $db->exec($cmd);
    if ($success === false) throw new Exception("exec() failed for " . $cmd);

    // Delete the metrics of the project
    $cmd = 'DELETE FROM _Metric WHERE RefProject = ' . $refProject;
    $success = $db->exec($cmd);
    if ($success === false) throw new Exception("exec() failed for " . $cmd);

    // Delete the project
    $cmd = 'DELETE FROM _Project WHERE Ref = ' . $refProject;
    $success = $db->exec($cmd);
    if ($success === false) throw new Exception("exec() failed for " . $cmd);

    // Set the success code in the result dictionary
    $res["ret"] = "0";

  } catch (Exception $e) {

    $res["ret"] = "1";
    $res["errMsg"] = "line " . $e->getLine() . ": " . $e->getMessage();

  }

  // Return the dictionary
  return $res;

}

// -------------------------------- Main block --------------------------

try {

  // Try to open the database
  try {

    $db =
      new SQLite3(
        $pathDB,
        SQLITE3_OPEN_READWRITE);

  // If we couldn't open it, it means it doesn't exist yet
  } catch (Exception $e) {

    // Create the database
    $db =
      CreateDatabase(
        $pathDB,
        $versionDB);

  }

  // Automatically upgrade the database if necessary
  UpgradeDB(
    $db,
    $versionDB);

  // If an action has been requested
  if (isset($_POST["action"])) {

    // If the user requested the version
    if ($_POST["action"] == "version") {

      $res = GetVersion($db);
      echo json_encode($res);

    // If the user requested to add a project
    } else if ($_POST["action"] == "add_project" and 
               isset($_POST["label"])) {

      $res = AddProject(
        $db,
        $_POST["label"]);
      echo json_encode($res);

    // If the user requested the list of projects
    } else if ($_POST["action"] == "projects") {

      $res = GetProjects($db);
      echo json_encode($res);

    // If the user requested to add a metric
    } else if ($_POST["action"] == "add_metric" and 
               isset($_POST["project"]) and
               isset($_POST["label"]) and
               isset($_POST["default"])) {

      $res =
        AddMetric(
          $db,
          $_POST["project"],
          $_POST["label"],
          $_POST["default"]);
      echo json_encode($res);

    // If the user requested the list of metrics
    } else if ($_POST["action"] == "metrics" and 
               isset($_POST["project"])) {

      $res =
        GetMetrics(
          $db,
          $_POST["project"]);
      echo json_encode($res);

    // If the user requested to add a measure
    } else if ($_POST["action"] == "add_measure" and 
               isset($_POST["project"])) {

      $res =
        AddMeasure(
          $db,
          $_POST["project"],
          $_POST);
      echo json_encode($res);

    // If the user requested to delete a measure
    } else if ($_POST["action"] == "delete_measure" and 
               isset($_POST["measure"])) {

      $res =
        DeleteMeasure(
        $db,
        $_POST["measure"]);
      echo json_encode($res);

    // If the user requested the data in JSON format
    } else if ($_POST["action"] == "measures" and 
               isset($_POST["project"])) {

      // If the user hasn't specified a limit for the number of returned
      // measure, set it by default to 0
      if (!isset($_POST["last"])) $_POST["last"] = 0;
      $res =
        GetMeasures(
          $db,
          $_POST["project"],
          $_POST["last"]);
      echo json_encode($res);

    // If the user requested the data in csv format
    } else if ($_POST["action"] == "csv" and 
               isset($_POST["project"])) {

      // If the user hasn't specified a separator, used & by default
      if (!isset($_POST["sep"])) $_POST["sep"] = '&';
      $res =
        GetMeasuresAsCSV($db,
        $_POST["project"],
        $_POST["sep"]);
      echo $res;

    // If the user requested to delete a project
    } else if ($_POST["action"] == "flush" and 
               isset($_POST["project"])) {

      $res =
        FlushProject(
          $db,
          $_POST["project"]);
      echo json_encode($res);

    // If the user requested the help
    } else if ($_POST["action"] == "help") {

      echo '{"ret":"0","actions":"version, ' . 
        'add_project&label=..., ' .
        'projects, ' .
        'add_metric&project=...&label=...&default=..., ' .
        'metrics&project=..., ' .
        'add_measure&project=...&...=...&..., ' .
        'delete_measure&measure=..., ' .
        'measures&project=...[&last=...(default: 0)], ' .
        'csv&project=...[&sep=...(default: &)], ' .
        'flush&project=..."}';

    // If the user requested an unknown or invalid action
    } else {

      echo '{"ret":"1","errMsg":"Invalid action"}';

    }

  // Else, nothing to do
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
