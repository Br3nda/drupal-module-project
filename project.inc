<?php
// $Id: project.inc,v 1.2 2003/09/28 17:10:01 kjartan Exp $

function project_project_form(&$node, &$help, &$error) {
  if (count($node->components)) {
    $node->components = implode("\n", $node->components);
  }
  else {
    $node->components = "Code\nDocumentation\nMiscellaneous\nUser interface";
  }

  if (function_exists("taxonomy_node_form")) {
    $output = implode("", taxonomy_node_form("project", $node));
  }
  //$output .= form_textfield(t("Full project name"), "title", $node->title, 40, 255, t("This is the publicly visible project name."). theme_error($error["title"]));
  $output .= form_textfield(t("Short project name"), "uri", $node->uri, 40, 50, t("This will be used to generate a /projects/&lt;shortname&gt;/ URL for your project."). theme_error($error["uri"]));
  $output .= form_textarea(t("Full description"), "body", $node->body, 40, 10, theme_error($error["body"]));
  $output .= form_textfield(t("Project e-mail"), "mail", $node->mail, 20, 255, t("E-mail address where the project owners can be contacted."). theme_error($error["mail"]));
  $output .= form_textfield(t("Homepage"), "homepage", $node->homepage, 40, 255, t("Link to project homepage."). theme_error($error["homepage"]));
  $output .= form_textfield(t("Changelog"), "changelog", $node->changelog, 40, 255, t("Link to changelog."). theme_error($error["changelog"]));
  $output .= form_textfield(t("CVS tree"), "cvs", $node->cvs, 40, 255, t("Link to webcvs/viewcvs."). theme_error($error["cvs"]));
  $output .= form_textfield(t("Demo site"), "demo", $node->demo, 40, 255, theme_error($error["demo"]));
  if (variable_get("project_release_directory", "")) {
    $output .= form_textfield(t("Release directory"), "release_directory", $node->release_directory, 40, 255, theme_error($error["release_directory"]));
  }

  $output .= "<hr />";
  $output .= form_item(t("Issue tracker"), form_checkbox(t("Enable issue tracker"), "issues", 1, $node->issues, t("Let users submit bug requests, patches, feature requests, support requests, etc.")));
  $output .= form_checkbox(t("Send monthly reminders to users"), "mail_reminder", 1, $node->mail_reminder, t("Enabling this will send a monthly reminder to users that have open issues registered."));
  $output .= form_textfield(t("Weekly critical issues report"), "mail_digest", $node->mail_digest, 20, 255, t("To get a weekly digest of critical issues specify an e-mail address.") . theme_error($error["mail_digest"]));
  $output .= form_textfield(t("Issues e-mail address"), "mail_copy", $node->mail_copy, 20, 255, t("If you wish to receive a copy of all the issues to a central location specify an address here.") . theme_error($error["mail_copy"]));
  $output .= form_textarea(t("Components"), "components", $node->components, 20, 5);
  $output .= form_select(t("Default version"), "version", $node->version, project_release_load($node));
  return $output;
}

function project_project_validate(&$node) {
  // Make sure title isn't already in use
  if (db_num_rows(db_query("SELECT nid FROM {node} WHERE type = '%s' AND status = 1 AND title = '%s' AND nid <> %d", $node->type, $node->title, $node->nid))) {
    $error["title"] .= t("This project name is already in use.");
  }

  // Validate uri.
  if (empty($node->uri)) {
    $error["uri"] = t("Required.");
  }
  else {
    // Make sure uri only includes valid characters
    if (!preg_match('/^[a-zA-Z0-9_]+$/', $node->uri)) {
      $error["uri"] = t("Please only use alphanumerical characters.");
    }

    // Make sure uri isn't already in use, or reserved
    if (in_array($node->uri, array("user", "issues", "releases")) || db_num_rows(db_query("SELECT nid FROM {project_projects} WHERE uri = '%s' AND nid <> %d", $node->uri, $node->nid))) {
      $error["uri"] .= t("This project name is already in use.");
    }
  }
  if (!$error["uri"]) {
    $node->path = "project/$node->uri";
  }

  // We need a description.
  if (empty($node->body)) {
    $error["body"] = t("Required.");
  }

  // Validate emails
  if (empty($node->mail)) {
    $error["mail"] = t("Required.");
  }
  elseif (($data = user_validate_mail($node->mail))) {
    $error["mail"] = $data;
  }

  if (($data = user_validate_mail($node->mail_digest))) {
    $error["mail_digest"] = $data;
  }

  if (($data = user_validate_mail($node->mail_copy))) {
    $error["mail_copy"] = $data;
  }

  // Make sure all URL fields actually contain URLs.
  foreach (array("homepage", "changelog", "cvs", "demo") as $uri) {
    if ($node->$uri && !preg_match('/^(http|https|ftp):\/\//i', $node->$uri)) {
      $error[$uri] = t("Invalid URL.");
    }
  }

  // Transform components into array.
  if (!is_array($node->components)) {
    if ($node->components) {
      $node->components = explode("\n", str_replace("\r", "", $node->components));
    }
    else {
      $node->components = array();
    }
  }

  return $error;
}

function project_project_view($node, $main = 0) {
  if ($main) {
    theme("node", $node, $main);
  }
  else {
    $node->breadcrumb[] = l(t("Home"), NULL);
    $node->breadcrumb[] = l(t("Projects"), "project");

    if ($output = project_release_view($node)) {
      $node->body .= "<h2>". t("Releases") ."</h2>";
      $node->body .= $output;
    }

    if (!variable_get("project_release_directory", "") && node_access($node, "update")) {
      $items[] = l(t("Add new release"), "project/$node->uri/releases/add");
    }
    if ($node->issues && user_access("access project issues")) {
      $items[] = l(t("View recently submitted issues"), "project/$node->uri/issues");
      $items[] = l(t("Search existing issues"), "project/$node->uri/issues/search");
      if (user_access("maintain project issues")) {
        foreach (project_issue_category() as $category => $name) {
          $items[] = l(t("Submit new %category", array("%category" => $name)), "node/add/project_issue", array(), "project=$node->uri&category=$category");
        }
      }
      $output = theme("theme_item_list", $items, t("Operations"));

      if ($GLOBALS["user"]->uid) {
        $level = db_result(db_query("SELECT level FROM {project_subscriptions} WHERE nid = %d AND uid = %d", $node->nid, $GLOBALS["user"]->uid));
        $form = form_radios(t("Issues e-mail subscription"), "level", $level, array(0 => t("None"), 1 => t("Own issues"), 2 => t("All issues")));
        $form .= form_submit(t("Subscribe"));
        $output .= form($form, "post", "project/$node->uri");
      }
    }

    theme("breadcrumb", $node->breadcrumb);
    theme("node", $node, $main);
    theme("box", NULL, $output);
  }
}

function project_project_load($node) {
  $node = db_fetch_object(db_query("SELECT * FROM {project_projects} WHERE nid = %d", $node->nid));

  $node->components = unserialize($node->components);

  return $node;
}

function project_project_insert($node) {
  $node->components = serialize($node->components);

  db_query("INSERT INTO {project_projects} (nid, uri, homepage, changelog, cvs, demo, release_directory, issues, components, mail, mail_digest, mail_reminder, mail_copy, version) VALUES (%d, '%s', '%s', '%s', '%s', '%s', '%s', %d, '%s', '%s', '%s', %d, '%s', %d)", $node->nid, $node->uri, $node->homepage, $node->changelog, $node->cvs, $node->demo, $node->release_directory, $node->issues, $node->components, $node->mail, $node->mail_digest, $node->reminder, $node->mail_copy, $node->version);
}

function project_project_update($node) {
  $node->components = serialize($node->components);

  db_query("UPDATE {project_projects} SET uri = '%s', homepage = '%s', changelog = '%s', cvs = '%s', demo = '%s', release_directory = '%s', issues = %d, components = '%s', mail = '%s', mail_digest = '%s', mail_reminder = %d, mail_copy = '%s', version = %d WHERE nid = %d", $node->uri, $node->homepage, $node->changelog, $node->cvs, $node->demo, $node->release_directory, $node->issues, $node->components, $node->mail, $node->mail_digest, $node->mail_reminder, $node->mail_copy, $node->version, $node->nid);
}

function project_project_delete($node) {
  db_query("DELETE FROM {project_projects} WHERE nid = %d", $node->nid);
}

function project_project_access($op, $node) {
  global $user;

  switch ($op) {
    case "view":
      return user_access("access projects") && $node->status;
    case "create":
      return user_access("maintain project") && $user->uid;
    case "update":
    case "delete":
      return (user_access("maintain project") && $user->uid == $node->uid) || user_access("administer projects");
  }
}

?>