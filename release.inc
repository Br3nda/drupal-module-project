<?php
// $Id: release.inc,v 1.15 2003/11/10 18:46:08 kjartan Exp $

function project_release_cron() {
  // TODO: should be able to optimize this to use less file system calls.
  if ($dir = variable_get("project_release_directory", "")) {
    $result = db_query("SELECT nid, 'project_project' AS type, release_directory FROM {project_projects} WHERE release_directory <> ''");
    while ($project = db_fetch_object($result)) {
      if ($handle = opendir($dir)) {
        $releases = project_release_load($project, 0);
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
        watchdog("error", "project: releases cron failed to open $dir");
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
        print theme("header", "Add release to $node->title");
        print theme("box", "Add release to $node->title", form($output, ($param["method"] ? $param["method"] : "post"), $param["action"], $param["options"]));
        print theme("footer");
      }
      break;
    case "edit":
      if (node_access($node, "update") && ($release = project_release_load(arg(4))) && $node->nid == $release->nid) {
        $output = project_release_form($release, $error, $param);
        print theme("header", "Edit release $release->version for $node->title");
        project_release_view($release);
        print theme("box", "Edit release", form($output, ($param["method"] ? $param["method"] : "post"), $param["action"], $param["options"]));
        print theme("footer");
      }
      break;
    case "view":
      if (node_access($node, "view") && ($release = project_release_load(arg(4))) && $node->nid == $release->nid) {
        print theme("header", "$node->title $release->version");
        project_release_view($release);
        print theme("footer");
      }
      break;
    case "delete":
      if (node_access("update", $node)) {
        if (($release = project_release_load(arg(4))) && $release->nid == $node->nid) {
          project_release_delete(arg(4));
          drupal_goto(url("node/view/$node->nid"));
          break;
        }
      }
      print theme("header");
      print theme("box", "", message_access());
      print theme("footer");
      break;
    case t("Preview"):
      if (node_access($node, "update")) {
        $edit = array2object($_POST["edit"]);
        $edit->nid = $node->nid;
        $edit->rid = arg(4);
        $error = project_release_validate($edit);
        $output = project_release_form($edit, $error, $param);

        print theme("header", $node->title);
        project_release_view($edit);
        print theme("box", t("Preview"), form($output, ($param["method"] ? $param["method"] : "post"), $param["action"], $param["options"]));
        print theme("footer");
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

          print theme("header", $node->title);
          print theme("box", $node->title, form($output, ($param["method"] ? $param["method"] : "post"), $param["action"], $param["options"]));
          print theme("footer");
        }
        else {
          // Save the release and re-direct to the overview page for the project
          if (variable_get("project_release_directory", "")) {
            unset($edit->version);
          }

          $edit->status = 1;
          project_release_save($edit);
          drupal_goto(url("node/view/$node->nid"));
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
      print theme("header", t("%project releases", array("%project" => "$node->title")));
      print theme("box", t("Releases"), project_release_view($node));
      print theme("footer");
  }
}

function project_release_overview() {
  $output = "<div class=\"project\">";
  $output .= "<div class=\"downloads\">";

  $header = array(t("project"), t("downloads"), t("description"));

  $result = db_query("SELECT DISTINCT(n.nid), n.title, n.teaser, r.tid FROM {node} n INNER JOIN {term_node} r ON n.nid = r.nid WHERE n.type = 'project_project' AND n.status = 1 ORDER BY r.tid, n.title");
  while ($node = db_fetch_object($result)) {
    if ($tid != ($tid = $node->tid)) {
      if (count($rows)) {
        $output .= "<h2>$term->name</h2>";
        $output .= "<p style=\"color: #888;\"><small>$term->description</small></p>";
        $output .= table($header, $rows);
      }
      $term = taxonomy_get_term($tid);
      $rows = array();
    }
    $releases = array();
    $result2 = db_query("SELECT path, version FROM {project_releases} WHERE nid = %d AND status = 1 AND created > 0 AND path <> '' ORDER BY created DESC", $node->nid);
    if (db_num_rows($result2)) {
      while ($release = db_fetch_object($result2)) {
        $releases[] = l($release->version, $release->path, array("class" => count($releases) ? "project-old" : "project-recent"));
      }
      $rows[] = array(
        array("class" => "project-name", "data" => l($node->title, "node/view/$node->nid")),
        array("class" => "project-download", "data" => theme("item_list", $releases)),
        array("class" => "project-teaser", "data" => strip_tags($node->teaser)),
      );
    }
  }
  if (count($rows)) {
    $output .= "<h2>$term->name</h2>";
    $output .= "<p style=\"color: #888;\"><small>$term->description</small></p>";
    $output .= table($header, $rows);
  }

  $output .= "</div>";
  $output .= "</div>";

  $breadcrumb[] = l(t("Home"), NULL);
  $breadcrumb[] = l(t("Projects"), "project");

  print theme("header");
  print theme("breadcrumb", $breadcrumb);
  print theme("box", t("Project downloads"), $output);
  print theme("footer");
}

function project_release_form(&$release, &$error, &$param) {
  /* TODO: pending final fileapi
  // Set form parameters so we can accept file uploads.
  $param["options"] = array("enctype" => "multipart/form-data");
  */

  $output = form_textfield(t("Version"), "version", $release->version, 40, 255, theme_error($error["version"]));
  $output .= form_textarea(t("Changes"), "changes", $release->changes, 40, 10, theme_error($error["changes"]));
  if (!variable_get("project_release_directory", "")) {
  /* TODO: pending final fileapi
  $output .= form_file(t("File"), "file", 40, ($release->fid) ? t("A file already exists, if you upload another file the current file will be replaced.") : "" . $error["file"], $release->fid);
  */
  }
  if ($release->rid) {
    $output .= form_hidden("rid", $release->rid);
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
    $error["version"] = t("Required.");
  }
  else {
    $result = db_query("SELECT * FROM {project_releases} WHERE nid = %d AND version = '%s'", $edit->nid, trim($edit->version));
    if ($release = db_fetch_object($result)) {
      if ($edit->rid != $release->rid) {
        if ($release->status) {
          $error["version"] = t("This version already exists for this project.");
        }
        else {
          $edit->rid = $release->rid;
        }
      }
    }
  }

  if (empty($edit->changes)) {
    $error["changes"] = t("Required.");
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

function project_release_view($project, $main = 0) {
  if ($project->changes) {
    $release = $project;
    $project = node_load(array("nid" => $project->nid));
    $links = array();
    if (node_access($project, "update")) {
      $links[] = l(t("edit"), "project/$project->uri/releases/edit/$project->rid");
    }
    $breadcrumb[] = l(t("Home"), NULL);
    $breadcrumb[] = l(t("Projects"), "project");
    if ($term = reset(taxonomy_node_get_terms($project->nid))) {
      $breadcrumb[] = l($term->name, "project", NULL, "tid=$term->tid");
    }
    $breadcrumb[] = l($project->title, "node/view/$project->nid");

    print theme("breadcrumb", $breadcrumb);
    print theme("box", "$project->title $release->version", check_output($release->changes));
    if ($links) {
      print "<p>". theme("links", $links) ."</p>";
    }
  }
  else {
    $result = db_query("SELECT * FROM {project_releases} WHERE nid = %d AND status = 1 ORDER BY created DESC", $project->nid);
    if (db_num_rows($result)) {
      $header = array(t("version"), t("released"), t("changes"));
      if (node_access($project, "update") || node_access($project, "delete")) {
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
          $row[] = ($release->fid) ? l($release->version, "project/$project->uri/releases/download/$release->rid", array("title" => t("Download: %size", array("%size" => format_size($file->size))))) : $release->version;
        }
        $row[] = ($release->created) ? array("data" => format_date($release->created, "custom", "d M Y"), "style" => "vertical-align: top;") : "";
        $row[] = ($release->changes) ? l(t("changes"), "project/$project->uri/releases/view/$release->rid") : "";
        if (node_access($project, "update")) {
          $row[] = l("edit", "project/$project->uri/releases/edit/$release->rid");
        }
        if (node_access($project, "delete")) {
          $row[] = l("delete", "project/$project->uri/releases/delete/$release->rid");
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

function project_release_load($rid, $status = 1) {
  if ($rid->type == "project_project") {
    $status = $status ? "AND status = 1" : "";
    $result = db_query("SELECT * FROM {project_releases} WHERE nid = %d $status ORDER BY created DESC", $rid->nid);
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
  $fields = array("rid", "nid", "fid", "path", "created", "changed", "version", "changes", "status");
  $node = node_load(array("nid" => $release->nid));

  /* TODO: pending final fileapi
  $file = check_file("file");
  */
  if (empty($release->rid)) {
    watchdog("special", "project: added release '$release->version' to $node->title", l("view post", "node/view/$release->nid"));
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
    watchdog("special", "project: updated release '$node->title $release->version'", l("view post", "node/view/$release->nid"));
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
  watchdog("special", "project: deleted release '$release->version'", l("view post", "node/view/$release->nid"));
  db_query("UPDATE {project_releases} SET status = 0 WHERE rid = %d", $rid);
}

?>
