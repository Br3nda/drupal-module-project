<?php
// $Id: comment.inc,v 1.17 2003/11/13 19:55:44 dries Exp $

function project_comment_page($project) {
  switch ($_POST["op"] ? $_POST["op"] : arg(2)) {
    case "add":
      if (user_access("maintain project issues")) {
        $node = node_load(array("nid" => arg(3), "type" => "project_issue"));

        foreach (array("nid", "pid", "rid", "category", "component", "priority", "assigned", "state", "title") as $var) {
          $comment->$var = $node->$var;
        }

        $error = project_comment_validate($comment);
        $output = project_comment_form($comment, $error, $param);
        print theme("header", t("New comment"));
        print theme("box", t("New comment"), form($output, ($param["method"] ? $param["method"] : "post"), $param["action"], $param["options"]));
        node_view($node);
        print theme("footer");
      }
      break;
    case "edit":
      if (user_access("maintain project issues")) {
        $node = node_load(array("nid" => arg(3), "type" => "project_issue"));
        $output = project_comment_form($comment, $error, $param);
        print theme("header", "Edit comment $comment->version for $node->title");
        project_comment_view($comment);
        print theme("box", "Edit comment", form($output, ($param["method"] ? $param["method"] : "post"), $param["action"], $param["options"]));
        print theme("footer");
      }
      break;
    case "delete":
      print theme("header", "TODO");
      print theme("box", "TODO", "This feature hasn't been implemented yet due to security considerations.");
      print theme("footer");
      break;
    case t("Preview"):
      if (user_access("maintain project issues")) {
        $edit = array2object($_POST["edit"]);
        $edit->nid = $node->nid;
        $edit->type = "project_issue";
        $error = project_comment_validate($edit);

        /*$rows[] = array(array("data" => t("Comment submitted by %user on %date", array("%user" => format_name($edit), "%date" => format_date(time()))), "colspan" => 2, "class" => "banner"));
        $rows[] = array(array("data" => "<p>". check_output($edit->body) ."</p>", "colspan" => 2, "class" => "content"));
        if ($edit->fid) {
          $rows[] = array(l(t("download attachment"), "project/$node->pid/comment/download/$edit->cid"));
        }
        $output .= "<div class=\"project\">";
        $output .= theme("table", NULL, $rows);
        $output .= "</div>";
        */
        $output = project_comment_form($edit, $error, $param);

        print theme("header", $edit->title);
        node_preview($edit);
        print theme("box", $node->edit, form($output, ($param["method"] ? $param["method"] : "post"), $param["action"], $param["options"]));
        print theme("footer");
      }
      break;
    case t("Submit"):
      if (user_access("maintain project issues")) {
        $node = node_load(array("nid" => arg(3), "type" => "project_issue"));
        $edit = array2object($_POST["edit"]);
        $edit->nid = $node->nid;
        $error = project_comment_validate($edit);
        if ($error) {
          $output = project_comment_form($edit, $error, $param);

          print theme("header", "$node->title");
          print theme("box", "$node->title", form($output, ($param["method"] ? $param["method"] : "post"), $param["action"], $param["options"]));
          print theme("footer");
        }
        else {
          // Save the comment and re-direct to the overview page for the project
          project_comment_save($edit);
          drupal_goto(url("node/view/$node->nid"));
        }
      }
      break;
    /* TODO: pending final fileapi
    case "download":
      $comment = project_comment_load(arg(3));
      if ($comment->fid) {
        drupal_file_send($comment->fid);
      }
      break;
    */
  }
}

function project_comment_form(&$edit, &$error, &$param) {
  $output = "<div class=\"node-form\">";
  $output .= "<div class=\"standard\">";
  $output .= form_textfield(t("Title"), "title", $edit->title, 60, 128, $error["title"]);
  $output .= project_issue_form($edit, $help, $error, $param);
  $output .= "</div></div>";

  if ($edit->cid) {
    $output .= form_hidden("cid", $edit->cid);
  }

  $output .= form_submit(t("Preview"));
  if (!$error) {
    $output .= form_submit(t("Submit"));
  }

  return $output;
}

function project_comment_validate(&$edit) {
  global $user;

  $edit->uid = $user->uid;
  $edit->name = $user->name;

  if ($edit->cid) {
    $comment = project_comment_load($edit->cid);
    $edit->nid = $comment->nid;
    $edit->fid = $comment->fid;
  }

  $error = project_issue_validate($edit);
  unset($error["body"]);

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

function project_comment_view($node, $main = 0) {
  global $user;
  $links = array();
  $rows = array();
  $fields = array(
    "title" => "Title",
    "pid" => "Project",
    "rid" => "Version",
    "component" => "Component",
    "category" => "Category",
    "priority" => "Priority",
    "assigned" => "Assigned to",
    "state" => "Status"
  );

  $result = db_query("SELECT p.*, u.name FROM {project_comments} p LEFT JOIN {users} u USING (uid) WHERE p.nid = %d ORDER BY p.created ASC", $node->nid);
  if (db_num_rows($result)) {
    while ($comment = db_fetch_object($result)) {
      $data = unserialize($comment->data);
      $summary = array();
      foreach ($fields as $field => $text) {
        if (isset($data["old"]->$field) && isset($data["new"]->$field)) {
          $summary[] = array(
            t($text) .":",
            project_mail_summary($field, $data["old"]->$field),
            "&raquo; ". project_mail_summary($field, $data["new"]->$field)
          );
        }
      }

      if ($summary || $comment->body) {
        $rows[] = array(
          array("class" => "header", "data" => t("%count submitted by %user on %date", array("%count" => "<a name=\"comment". ++$i. "\" href=\"". url("node/view/$node->nid") ."#$i\">#$i</a>", "%user" => format_name($comment), "%date" => format_date($comment->created))) . (node_is_new($node->nid, $comment->changed) ? theme_mark() : "")));
        if ($summary) {
          $rows[] = array(array("class" => "summary", "data" => theme("table", NULL, $summary)));
        }
        if ($comment->body) {
          $rows[] = array(array("class" => "content", "data" => "<p>". check_output($comment->body) ."</p>"));
        }
        if ($comment->fid) {
          $rows[] = array(l(t("download attachment"), "project/comments/download/$comment->cid"));
        }
      }
    }
    $output = "<div class=\"project\">";
    $output .= theme("table", NULL, $rows);
    $output .= "</div>";

    if (user_access("maintain project issues")) {
      $links[] = "<p>". l(t("update issue"), "project/comments/add/$node->nid") ."</p>";
    }
    $output .=  theme("links", $links);
    print theme("box", t("Updates"), $output);
  }
}

function project_comment_load($cid) {
  return db_fetch_object(db_query("SELECT p.*, u.name FROM {project_comments} p LEFT JOIN {users} u USING (uid) WHERE p.cid = %d ORDER BY timestamp DESC", $cid));
}

function project_comment_save($edit) {
  global $user;

  /* TODO: pending final fileapi
  $file = check_file("file");
  */
  if (empty($edit->cid)) {
    $edit->cid = db_next_id("project_cid");
    /* TODO: pending final fileapi
    if ($file) {
      $edit->fid = drupal_file_save($file);
    }
    */

    if (empty($edit->uid)) {
      $edit->uid = $user->uid;
    }
    $node = node_load(array("nid" => $edit->nid, "type" => "project_issue"));

    // Check if comment changed any of the state values and update the node if necessary
    foreach (array("pid", "rid", "category", "component", "priority", "assigned", "state", "title") as $var) {
      if ($node->$var != $edit->$var) {
        $data["old"]->$var = $node->$var;
        $data["new"]->$var = $edit->$var;
        $node->$var = $edit->$var;
      }
    }

    watchdog("special", "project: added comment '$edit->title'", l("view post", "node/view/$node->nid"));
    db_query("INSERT INTO {project_comments} (cid, nid, uid, fid, created, changed, body, data) VALUES (%d, %d, %d, %d, %d, %d, '%s', '%s')", $edit->cid, $edit->nid, $edit->uid, $edit->fid, time(), time(), $edit->body, serialize($data));
    node_save($node);
  }

  return $edit->cid;
}

?>
