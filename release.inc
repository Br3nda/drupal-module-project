<?php
// $Id: release.inc,v 1.8 2003/10/03 11:59:00 kjartan Exp $

function project_release_cron() {
  if ($dir = variable_get("project_release_directory", "")) {
    $result = db_query("SELECT nid, 'project_project' AS type, release_directory FROM {project_projects} WHERE release_directory <> ''");
    while ($project = db_fetch_object($result)) {
      if ($handle = opendir($dir)) {
        $releases = project_release_load($project);
        while ($version = readdir($handle)) {
          if (is_dir("$dir/$version") && !in_array($version, array(".", "..", "CVS"))) {
            if (file_exists(getcwd() ."/$dir/$version/$project->release_directory-$version.tar.gz")) {
              if ($rid = array_search($version, $releases)) {
                unset($releases[$rid]);
              }
              else {
                $release->nid = $project->nid;
                $release->fid = 0;
                $release->version = $version;
                $release->path = "$dir/$version/$project->release_directory-$version.tar.gz";
                project_release_save($release);
              }
            }
          }
        }
        foreach ($releases as $rid => $version) {
          project_release_delete($rid);
        }
      }
      else {
        watchdog("error", "project releases: Failed to open $dir");
      }
    }
  }
}

function project_release_page($node) {
  switch ($_POST["op"] ? $_POST["op"] : arg(3)) {
    case "add":
      if (node_access($node, "update")) {
        $error = 1;
        $output = project_release_form($release, $error, $param);
        theme("header", "Add release to $node->title");
        theme("box", "Add release to $node->title", form($output, ($param["method"] ? $param["method"] : "post"), $param["action"], $param["options"]));
        theme("footer");
      }
      break;
    case "edit":
      if (node_access($node, "update") && ($release = project_release_load(arg(4))) && $node->nid == $release->nid) {
        $output = project_release_form($release, $error, $param);
        theme("header", "Edit release $release->version for $node->title");
        project_release_view($release);
        theme("box", "Edit release", form($output, ($param["method"] ? $param["method"] : "post"), $param["action"], $param["options"]));
        theme("footer");
      }
      break;
    case "view":
      if (node_access($node, "view") && ($release = project_release_load(arg(4))) && $node->nid == $release->nid) {
        theme("header", "$node->title $release->version");
        project_release_view($release);
        theme("footer");
      }
      break;
    case "delete":
      theme("header", "TODO");
      theme("box", "TODO", "This feature hasn't been implemented yet due to security considerations.");
      theme("footer");
      break;
    case t("Preview"):
      if (node_access($node, "update")) {
        $edit = array2object($_POST["edit"]);
        $edit->nid = $node->nid;
        $edit->rid = arg(4);
        $error = project_release_validate($edit);
        $output = project_release_form($edit, $error, $param);

        theme("header", $node->title);
        project_release_view($edit);
        theme("box", t("Preview"), form($output, ($param["method"] ? $param["method"] : "post"), $param["action"], $param["options"]));
        theme("footer");
      }
      break;
    case t("Submit"):
      if (node_access($node, "update")) {
        $edit = array2object($_POST["edit"]);
        $edit->nid = $node->nid;
        $edit->rid = arg(4);
        $error = project_release_validate($edit);
        if ($error) {
          $output = project_release_form($edit, $error, $param);

          theme("header", $node->title);
          theme("box", $node->title, form($output, ($param["method"] ? $param["method"] : "post"), $param["action"], $param["options"]));
          theme("footer");
        }
        else {
          // Save the release and re-direct to the overview page for the project
          if (variable_get("project_release_directory", "")) {
            unset($edit->version);
          }

          project_release_save($edit);
          drupal_goto(url("project/". arg(1)));
        }
      }
      break;
    /* TODO: pending final fileapi
    case "download":
      $release = project_release_load(arg(4));
      if ($release->fid) {
        drupal_file_send($release->fid);
      }
      break;
    */
    default:
      theme("header", t("%project releases", array("%project" => "$node->title")));
      theme("box", t("Releases"), project_release_view($node));
      theme("footer");
  }
}

function project_release_overview() {
  $output = "<div class=\"project\">";

  // Figure out which versions we have
  $versions = array();
  $result = db_query("SELECT version FROM project_releases WHERE path <> '' GROUP BY version");
  while ($version = db_result($result, $i++)) {
    $versions[] = $version;
  }

  $header = array_merge(array(t("project")), $versions);

  $result = db_query("SELECT DISTINCT(n.nid), n.title, r.tid FROM {node} n INNER JOIN {term_node} r ON n.nid = r.nid WHERE n.type = 'project_project' AND n.status = 1 ORDER BY r.tid, n.title");
  while ($node = db_fetch_object($result)) {
    $releases = array();
    $result2 = db_query("SELECT path, version FROM {project_releases} WHERE nid = %d AND path <> ''", $node->nid);
    while ($release = db_fetch_object($result2)) {
      $releases[$release->version] = $release->path;
    }
    if ($tid != ($tid = $node->tid)) {
      if (count($rows)) {
        $output .= "<h2>$term->name</h2>";
        $output .= "<p style=\"color: #888;\"><small>$term->description</small></p>";
        $output .= table($header, $rows);
      }
      $term = taxonomy_get_term($tid);
      $rows = array();
    }
    if (count($releases)) {
      $row = array(l($node->title, "project/$node->nid"));
      foreach ($versions as $version) {
        $row[] = ($releases[$version]) ? l(t("download"), $releases[$version]) : "";
      }
      $rows[] = $row;
    }
  }
  if (count($rows)) {
    $output .= "<h2>$term->name</h2>";
    $output .= "<p style=\"color: #888;\"><small>$term->description</small></p>";
    $output .= table($header, $rows);
  }

  $output .= "</div>";

  $breadcrumb[] = l(t("Home"), NULL);
  $breadcrumb[] = l(t("Projects"), "project");

  theme("header");
  theme("breadcrumb", $breadcrumb);
  theme("box", t("Project downloads"), $output);
  theme("footer");
}

function project_release_form(&$release, &$error, &$param) {
  /* TODO: pending final fileapi
  // Set form parameters so we can accept file uploads.
  $param["options"] = array("enctype" => "multipart/form-data");
  */

  if (!variable_get("project_release_directory", "")) {
    $output = form_textfield(t("Version"), "version", $release->version, 40, 255, $error["version"]);
    $output .= form_textarea(t("Changes"), "changes", $release->changes, 40, 10, $error["changes"]);
    /* TODO: pending final fileapi
    $output .= form_file(t("File"), "file", 40, ($release->fid) ? t("A file already exists, if you upload another file the current file will be replaced.") : "" . $error["file"], $release->fid);
    */
    if ($release->rid) {
      $output .= form_hidden("rid", $release->rid);
    }
  }
  else {
    $output = form_item(t("Version"), $release->version);
    $output .= form_hidden("version", $release->version);
    $output .= form_textarea(t("Changes"), "changes", $release->changes, 60, 20, $error["changes"]);
  }

  $output .= form_submit(t("Preview"));
  if (!$error) {
    $output .= form_submit(t("Submit"));
  }

  return $output;
}

function project_release_validate(&$edit) {
  if ($edit->rid) {
    $release = project_release_load($edit->rid);
    $edit->nid = $release->nid;
    $edit->fid = $release->fid;
  }

  if (empty($edit->version)) {
    $error["version"] = theme_error("Required.");
  }

  if (empty($edit->changes)) {
    $error["changes"] = theme_error("Required.");
  }

  /* TODO: pending final fileapi
  $file = check_file("file");

  if ($file && !$file->fid && !empty($file->error)) {
    $error["file"] = theme_error($file->error);
  }
  else {
    $edit->fid = $file->fid;
  }
  */

  return $error;
}

function project_release_view($node, $main = 0) {
  if ($node->changes) {
    $project = node_load(array("nid" => $node->nid));
    $links = array();
    if (node_access($project, "update")) {
      $links[] = l(t("edit"), "project/$project->uri/releases/edit/$node->rid");
    }
    $breadcrumb[] = l(t("Home"), NULL);
    $breadcrumb[] = l(t("Projects"), "project");
    if ($term = reset(taxonomy_node_get_terms($project->nid))) {
      $breadcrumb[] = l($term->name, "project", NULL, "tid=$term->tid");
    }
    $breadcrumb[] = l($project->title, "project/$project->uri");

    theme("breadcrumb", $breadcrumb);
    theme("box", "$project->title $node->version", check_output($node->changes));
    if ($links) {
      print "<p>". theme("links", $links) ."</p>";
    }
  }
  else {
    $result = db_query("SELECT * FROM {project_releases} WHERE nid = %d ORDER BY created DESC", $node->nid);
    if (db_num_rows($result)) {
      $header = array(t("version"), t("released"), t("changes"));
      if (node_access($node, "update") || node_access($node, "delete")) {
        $header[] = array("data" => t("operations"), "colspan" => 2);
      }
      while ($release = db_fetch_object($result)) {
        /* TODO: pending final fileapi
        if ($release->fid) {
          $file = drupal_file_load($release->fid);
        }
        */
        $row = array();
        if (variable_get("project_release_directory", "")) {
          $row[] = ($release->path) ? l($release->version, $release->path) : $release->version;
        }
        else {
          $row[] = ($release->fid) ? l($release->version, "project/$node->uri/releases/download/$release->rid", array("title" => t("Download: %size", array("%size" => format_size($file->size))))) : $release->version;
        }
        $row[] = ($release->created) ? array("data" => format_date($release->created, "custom", "d M Y"), "style" => "vertical-align: top;") : "";
        $row[] = ($release->changes) ? l(t("changes"), "project/$node->uri/releases/view/$release->rid") : "";
        if (node_access($node, "update")) {
          $row[] = l("edit", "project/$node->uri/releases/edit/$release->rid");
        }
        if (!variable_get("project_release_directory", "") && node_access($node, "delete")) {
          $row[] = l("delete", "project/$node->uri/releases/delete/$release->rid");
        }
        $rows[] = $row;
      }

      $output = "<div class=\"project\">\n";
      $output .= table($header, $rows);
      $output .= "</div>\n";

      return $output;
    }
  }
}

function project_release_load($rid) {
  if ($rid->type == "project_project") {
    $result = db_query("SELECT * FROM {project_releases} WHERE nid = %d ORDER BY created DESC", $rid->nid);
    $releases = array();
    while ($release = db_fetch_object($result)) {
      $releases[$release->rid] = $release->version;
    }
    return $releases;
  }
  elseif (is_numeric($rid)) {
    return db_fetch_object(db_query("SELECT * FROM {project_releases} WHERE rid = %d ORDER BY created DESC", $rid));
  }
}

function project_release_save($release) {
  $fields = array("rid", "nid", "fid", "path", "created", "changed", "version", "changes");

  /* TODO: pending final fileapi
  $file = check_file("file");
  */
  if (empty($release->rid)) {
    watchdog("special", "project release: added '$release->version'", l("view post", "node/view/$release->nid"));
    $release->rid = db_next_id("project_rid");
    if (!variable_get("project_release_directory", "") && $file->filename) {
      $release->fid = drupal_file_save($file);
    }

    $release->created = ($release->version == "cvs") ? 0 : time();
    $release->changed = time();

    // Prepare the query:
    foreach ($release as $key => $value) {
      if (in_array($key, $fields)) {
        $k[] = check_query($key);
        $v[] = $value;
        $s[] = "'%s'";
      }
    }

    // Insert the node into the database:
    db_query("INSERT INTO {project_releases} (". implode(", ", $k) .") VALUES(". implode(", ", $s) .")", $v);
  }
  else {
    watchdog("special", "project release: updated '$release->version'", l("view post", "node/view/$release->nid"));
    /* TODO: pending final fileapi
    // Replace current file if new file upload exists.
    if (!variable_get("project_release_directory", "") && $file->filename) {
      $file->fid = $release->fid;
      drupal_file_save($file);
    }
    */

    unset($release->created);
    $release->changed = time();

    // Prepare the query:
    foreach ($release as $key => $value) {
      if (in_array($key, $fields)) {
        $q[] = check_query($key) ." = '%s'";
        $v[] = $value;
      }
    }
    $v[] = $release->nid;
    $v[] = $release->rid;

    db_query("UPDATE {project_releases} SET ". implode(", ", $q) ." WHERE nid = %d AND rid = %d", $v);
  }
  return $release->rid;
}

function project_release_delete($rid) {
  watchdog("special", "project release: deleted '$release->version'", l("view post", "node/view/$release->nid"));
  db_query("DELETE FROM {project_releases} WHERE rid = %d", $rid);
}

function project_release_latest($nid) {
  return db_fetch_object(db_query("SELECT * FROM {project_releases} WHERE nid = %d ORDER BY rid DESC LIMIT 1", $nid));
}

?>
