<?php
// $Id: issue.inc,v 1.22.2.1 2003/10/25 18:48:21 kjartan Exp $

function project_issue_node($field) {
  $info["name"] = t("Issue");

  return $info[$field];
}

function project_issue_page($project) {
  $arg = ($project->nid) ? 3 : 2;

  switch ($_POST["op"] ? $_POST["op"] : arg($arg)) {
    case "view":
      if ($node = node_load(array("nid" => arg($arg + 1), "type" => "project_issue"))) {
        theme("header", $node->title);
        node_view($node);
        theme("footer");
      }
      break;
    case "download":
      $node = node_load(array("nid" => arg(4)));
      /* TODO: pending final fileapi
      if ($node->fid) {
        drupal_file_send($node->fid);
      }
      */
      break;
    case "search":
      theme("header", "Search $project->title");
      theme("box", "Search $project->title", project_issue_query($project, $query));
      theme("footer");
      break;
    case "rss":
      $output = project_issue_query_result($project, "rss");
      header("Content-Type: text/xml");
      print $output;
      break;
    default:
      $output = project_issue_query_result($project);

      theme("header");
      $breadcrumb[] = l(t("Home"), NULL);
      $breadcrumb[] = l(t("Projects"), "project");
      if (function_exists("taxonomy_node_get_terms") && $term = reset(taxonomy_node_get_terms($project->nid))) {
        $breadcrumb[] = l($term->name, "project", NULL, "tid=$term->tid");
      }
      $breadcrumb[] = $project->nid ? l($project->title, "node/view/$project->nid") : l(t("All issues"), "project/issues");

      theme("breadcrumb", $breadcrumb);
      theme("box", l(t("Issues"), $project->nid ? "project/$project->uri/issues" : "project/issues"), $output);
      theme("footer");
  }
}

function project_issue_form(&$node, &$help, &$error, &$param) {
  global $user;

  /* TODO: pending final fileapi
  // Set form parameters so we can accept file uploads.
  $param["options"] = array("enctype" => "multipart/form-data");
  */

  // Fetch a list of all projects to make swaping simpler
  $projects = array(t("<none>"));
  $result = db_query("SELECT n.nid, n.title FROM {project_projects} p LEFT JOIN {node} n USING (nid) WHERE n.status = 1 AND p.issues = 1");
  while ($project = db_fetch_object($result)) {
    $projects[$project->nid] = $project->title;
  }

  $output = form_select(t("Project"), "pid", $node->pid, $projects, theme_error($error["pid"]));

  if ($node->pid) {
    // Load the project and initialize some support arrays.
    $project = node_load(array("nid" => $node->pid, "type" => "project_project"));
    if ($releases = project_release_load($project)) {
      $releases = array(t("<none>")) + project_release_load($project);
    }
    if ($project->components) {
      $components = array(t("<none>"));
      foreach ($project->components as $component) {
        $components[$component] = $component;
      }
    }
    $categories = array_merge(t("<none>"), project_issue_category(0, 0));
    $priorities = project_issue_priority();
    $states = project_issue_state();
    if ($user->uid == $node->assigned) {
      $assigned = array(0 => t("Unassign"), $user->uid => $user->name);
    }
    else {
      $assigned = array(
        $node->assigned => ($node->assigned && ($account = user_load(array("uid" => $node->assigned))) ? $account->name : t("Unassigned")),
        $user->uid => $user->name
      );
    }

    $output .= form_select(t("Version"), "rid", $node->rid, $releases, theme_error($error["rid"]));
    $output .= form_select(t("Component"), "component", $node->component, $components, theme_error($error["component"]));
    $output .= form_select(t("Category"), "category", $node->category, $categories, theme_error($error["category"]));

    $output .= form_select(t("Priority"), "priority", $node->priority, $priorities);
    $output .= form_select(t("Assigned"), "assigned", $node->assigned, $assigned);
    if ($node->state || $node->nid) {
      $output .= form_select(t("Status"), "state", $node->state, $states);
    }
    $output .= form_textarea("Description", "body", $node->body, 60, 20, theme_error($error["body"]));
    /* TODO: pending final fileapi
    $output .= form_file(t("File attachment"), "file", 40, ($node->fid) ? t("A file already exists, if you upload another file the current file will be replaced.") : "" . theme_error($error["file"]), $node->fid);
    */
  }

  return $output;
}

function project_issue_validate(&$node) {
  // Try to find the active project
  if (empty($node->pid) && $_GET["project"]) {
    if (is_numeric($_GET["project"])) {
      $node->pid = db_result(db_query("SELECT nid FROM {project_projects} WHERE nid = %d", $_GET["project"]), 0);
    }
    else {
      $node->pid = db_result(db_query("SELECT nid FROM {project_projects} WHERE uri = '%s'", $_GET["project"]), 0);
    }
  }
  if ($node->pid && $project = node_load(array("nid" => $node->pid))) {
    if ($releases = project_release_load($project)) {
      if (!$node->rid || !$releases[$node->rid]) {
        $node->rid = $project->version;
      }
      empty($node->rid) and $error["rid"] = t("Required.");
    }
    if ($node->component && !in_array($node->component, $project->components)) {
      $node->component = 0;
    }
    empty($node->component) and $error["component"] = t("Required.");
    empty($node->priority) and $node->priority = 2;
    empty($node->category) and $node->category = $_GET["category"];
    empty($node->category) and $error["category"] = t("Required.");
    empty($node->state) and $node->state = 1;
    empty($node->body) and $error["body"] = t("Required.");
  }
  else {
    $error["pid"] = t("Required.");
  }

  /* TODO: pending final fileapi
  $file = check_file("file");

  if (!$file->fid && !empty($file->error)) {
    $error["file"] = theme_error($file->error);
  }
  else {
    $node->fid = $file->fid;
  }
  */

  return $error;
}

function project_issue_view($node, $main = 0) {
  if ($main) {
    theme("node", $node, $main);
  }
  else {
    $project = node_load(array("nid" => $node->pid, "type" => "project_project"));
    $release = project_release_load($node->rid);
    $assigned = user_load(array("uid" => $node->assigned));

    $output = "<div class=\"project\">";
    $rows = array();
    $rows[] = array("Project:", $project->title);
    if ($release->version) {
      $rows[] = array("Version:", $release->version);
    }
    $rows[] = array("Component:", check_output($node->component));
    $rows[] = array("Category:", check_output($node->category));
    $rows[] = array("Priority:", project_issue_priority($node->priority));
    $rows[] = array("Assigned:", $assigned->name);
    $rows[] = array("Status:", project_issue_state($node->state));
    $rows[] = array(array("data" => "<br />", "colspan" => 2));
    $output .= "<div class=\"summary\">". table(NULL, $rows) ."</div>";

    $rows = array();
    $rows[] = array(array("data" => t("Description"), "colspan" => 2, "class" => "banner"));
    $rows[] = array(array("data" => check_output($node->body), "colspan" => 2, "class" => "content"));
    $output .= table(NULL, $rows);
    $output .= "</div>";

    $node->body = $output;

    $breadcrumb[] = l(t("Home"), NULL);
    $breadcrumb[] = l(t("Projects"), "project");
    if (function_exists("taxonomy_node_get_terms") && $term = reset(taxonomy_node_get_terms($project->nid))) {
      $breadcrumb[] = l($term->name, "project", NULL, "tid=$term->tid");
    }
    $breadcrumb[] = l($project->title, "node/view/$project->nid");
    $breadcrumb[] = l(t("Issues"), "project/$project->uri/issues");

    theme("breadcrumb", $breadcrumb);
    theme("node", $node, $main);
    project_comment_view($node);
  }
}

function project_issue_load($node) {
  $project = db_fetch_object(db_query("SELECT * FROM {project_issues} WHERE nid = %d", $node->nid));
  return $project;
}

function project_issue_insert($node) {
  /* TODO: pending final fileapi
  if ($file->filename) {
    $node->fid = drupal_file_save($file);
  }
  */
  db_query("INSERT INTO {project_issues} (nid, pid, fid, category, component, priority, rid, assigned, state) VALUES (%d, %d, %d, '%s', '%s', %d, %d, %d, %d)", $node->nid, $node->pid, $node->fid, $node->category, $node->component, $node->priority, $node->rid, $node->assigned, $node->state);
  project_mail_notify($node);
}

function project_issue_update($node) {
  /* TODO: pending final fileapi
  if ($file->filename) {
    $node->fid = drupal_file_save($file);
  }
  */
  db_query("UPDATE {project_issues} SET pid = %d, fid = %d, category = '%s', component = '%s', priority = %d, rid = %d, assigned = %d, state = %d WHERE nid = %d", $node->pid, $file->fid, $node->category, $node->component, $node->priority, $node->rid, $node->assigned, $node->state, $node->nid);
}

function project_issue_delete($node) {
  db_query("DELETE FROM {project_issues} WHERE nid = %d", $node->nid);
}

function project_issue_access($op, $node) {
  switch ($op) {
    case "view":
      return $node->status && user_access("access project issues");
    case "create":
      return user_access("maintain project issues");
    case "update":
    case "delete":
      return user_access("administer projects");
  }
}

// Support stuff
function project_issue_state($state = 0) {
  $states = array(1 => t("active"), t("fixed"), t("duplicate"), t("postponed"), t("won't fix"), t("by design"), t("closed"));
  return $state ? $states[$state] : $states;
}

function project_issue_priority($priority = 0) {
  $priorities = array(1 => "critical", "normal", "minor");
  return $priority ? $priorities[$priority] : $priorities;
}

function project_issue_category($category = 0, $plural = 1) {
  if ($plural) {
    $categories = array("bug" => t("bug reports"), "task" => t("tasks"), "feature" => t("feature requests"), "support" => t("support requests"));
  }
  else {
    $categories = array("bug" => t("bug report"), "task" => t("task"), "feature" => t("feature request"), "support" => t("support request"));
  }
  return $category ? $categories[$category] : $categories;
}

function project_issue_count($pid) {
  $state = array();
  $result = db_query("SELECT p.state, count(p.state) AS count FROM {node} n LEFT JOIN {project_issues} p USING (nid) WHERE n.status = 1 AND p.pid = %d GROUP BY p.state", $pid);
  while ($data = db_fetch_object($result)) {
    $state[$data->state] = $data->count;
  }
  return $state;
}

function project_issue_query($project, $query) {
  $categories = project_issue_category();
  if ($project->nid) {
    foreach ($project->components as $component) {
      $components[$component] = $component;
    }
    $versions = project_release_load($project);
  }
  else {
    $result = db_query("SELECT nid, title FROM {node} WHERE type = 'project_project' AND status = 1 ORDER BY title");
    while ($project = db_fetch_object($result)) {
      $projects[$project->nid] = $project->title;
    }
  }
  $states = project_issue_state();
  $priorities = project_issue_priority();

  $rows[] = array(
    array("data" => form_textfield(t("Search for"), "text", $query->summary, 60, 255), "colspan" => 3)
  );
  if ($projects) {
    $rows[] = array(
      form_select(t("Projects"), "projects", $query->projects, $projects, "", "size=\"5\"", 1),
      form_select(t("Categories"), "categories", $query->categories, $categories, "", "size=\"5\"", 1),
    );
  }
  else {
    $rows[] = array(
      form_select(t("Versions"), "versions", $query->versions, $versions, "", "size=\"5\"", 1),
      form_select(t("Components"), "components", $query->components, $components, "", "size=\"5\"", 1),
      form_select(t("Categories"), "categories", $query->categories, $categories, "", "size=\"5\"", 1),
    );
  }
  $rows[] = array(
    form_select(t("Priorities"), "priorities", $query->priorities, $priorities, "", "size=\"5\"", 1),
    form_select(t("Status"), "states", ($query->states) ? $query->states : array(1, 2), $states, "", "size=\"5\"", 1)
  );
  $rows[] = array(
    array("data" => form_submit(t("Search")), "colspan" => 3)
  );

  $output = "<div class=\"project\">";
  $output .= form(table(NULL, $rows), "post", url($project->uri ? "project/$project->uri/issues" : "project/issues"));
  $output .= "</div>";

  return $output;
}

function project_issue_query_result($project = 0, $format = "html") {
  $states = project_issue_state();
  $priorities = project_issue_priority();

  // Get all projects we are viewing
  $projects = array();
  if ($project->nid) {
    $releases = project_release_load($project);
    $query->projects = $project->nid;
  }
  else {
    $result = db_query("SELECT nid, title FROM node WHERE type = 'project_project' AND status = 1");
    while ($node = db_fetch_object($result)) {
      $projects[$node->nid] = $node->title;
    }
  }

  if (!$project->nid) {
    $header[] = array("data" => t("project"), "field" => "p.pid");
  }
  $header[] = array("data" => t("summary"), "field" => "n.title");
  $header[] = array("data" => t("state"), "field" => "p.state");
  $header[] = array("data" => t("pri"), "field" => "p.priority");
  $header[] = array("data" => t("category"), "field" => "p.category");
  if (count($releases)) {
    $header[] = array("data" => t("version"), "field" => "p.rid");
  }
  $header[] = array("data" => t("last updated"), "field" => "n.changed", "sort" => "desc");
  $header[] = array("data" => t("assigned to"), "field" => "p.assigned");
  #$header[] = array("data" => t("submitted by"), "field" => "n.uid");

  if (($query = project_issue_query_parse($query))) {
    $sql = project_issue_query_sql($query);
    $result = pager_query($sql["sql"] . tablesort_sql($header), 20, 0, $sql["count"]);
  }
  else {
    $result = pager_query("SELECT n.nid FROM {project_issues} p LEFT JOIN {node} n USING (nid) WHERE n.status = 1 AND (". implode(" OR ", $pids) .") AND (p.state = 1 OR p.state = 2)". tablesort_sql($header), 20);
  }

  if ($format == "rss") {
    return project_issue_query_rss($projects, $result);
  }
  elseif (db_num_rows($result)) {
    while ($node = db_fetch_object($result)) {
      $node = node_load($node);
      $row = array();
      $number++;
      $class = "state_". ($number % 2 ? "light" : "dark") ."_$node->state";
      if ($projects) {
        $row[] = array("data" => l($projects[$node->pid], "node/view/$node->pid"), "class" => $class);
      }
      $row[] = array("data" => l(substr($node->title, 0, 50), "node/view/$node->nid") . (node_is_new($node->nid, $node->changed) ? theme_mark() ." " : ""), "class" => $class);
      $row[] = array("data" => $states[$node->state], "class" => $class);
      $row[] = array("data" => $priorities[$node->priority], "class" => $class);
      $row[] = array("data" => t($node->category), "class" => $class);
      if (count($releases)) {
        $row[] = array("data" => $releases[$node->rid], "class" => $class);
      }
      $row[] = array("data" => project_interval(time() - $node->changed, 2), "align" => "right", "class" => $class);
      $row[] = array("data" => ($node->assigned) ? format_name(user_load(array("uid" => $node->assigned))) : "", "class" => $class);

      $rows[] = $row;
    }
    $query = project_issue_query_pager($query);
    $rss = array();
    foreach ($query as $key => $value) {
      $rss[] = "$key=$value";
    }
    $rss = l("<img align=\"right\" src=\"misc/xml.gif\" />", "project/issues/rss", array("title" => t("RSS feed for these issues.")), implode("&amp;", $rss));
    if ($pager = pager_display(NULL, 20, 0, "default", $query + tablesort_pager())) {
      $rows[] = array(array("data" => $rss . $pager, "colspan" => count($headers)));
    }
    else {
      $rows[] = array(array("data" => $rss, "colspan" => count($headers)));
    }
  }
  else {
    $rows[] = array(array("data" => t("No issues found."), "colspan"=> 8));
  }

  $items = array(l(t("search issues"), ($project) ? "project/$project->uri/issues/search" : "project/issues/search"));
  if (user_access("maintain project issues")) {
    foreach (project_issue_category(0, 0) as $category => $name) {
      $items[] =  l(t("submit %category", array("%category" => $name)), "node/add/project_issue", array(), ($project->uri ? "project=$project->uri&" : "") . "category=$category");
    }
  }
  $rows[] = array(array("data" => theme("theme_item_list", $items), "colspan"=> 8));

  $output = "<div class=\"project\">";
  $output .= table($header, $rows);
  $output .= "</div>";
  return $output;
}

function project_issue_query_parse($query = NULL) {
  $fields = array("projects", "text", "summary", "comment", "categories", "components", "versions", "states", "priorities", "users", "assigned", "submitted");
  if (is_array($_POST["edit"])) {
    foreach ($_POST["edit"] as $key => $value) {
      if (!empty($value) && in_array($key, $fields)) {
        $query->$key = $value;
        $_POST[$key] = is_array($value) ? implode(",", $value) : $value;
      }
    }
    unset($_POST["edit"], $_POST["op"]);
  }
  else {
    foreach ($_GET as $key => $value) {
      if (!empty($value) && in_array($key, $fields)) {
        $query->$key = explode(",", $value);
      }
    }
  }
  if ($_SERVER['REQUEST_METHOD'] == 'GET' && empty($query->states)) {
    $query->states = array(1, 2);
  }
  return $query;
}

function project_issue_query_sql_field($field, $values, $like = 0, $operator = " OR ") {
  $sql = array();
  if (is_array($values)) {
    foreach ($values as $value) {
      $value = check_query($value);
      $sql[] = $like ? "$field LIKE '%$value%'" : "$field = '$value'";
    }
  }
  else {
    $value = check_query($values);
    $sql[] = $like ? "$field LIKE '%$value%'" : "$field = '$value'";
  }
  return "(". implode($operator, $sql) .")";
}

function project_issue_query_pager($query) {
  $get = array();
  if (count($query)) {
    foreach ($query as $key => $value) {
      $get[$key] = (is_array($value)) ? implode(",", $value) : $value;
    }
  }
  return $get;
}

function project_issue_query_sql($query) {
  foreach ($query as $key => $value) {
    switch ($key) {
      case "projects":
        $sql[] = project_issue_query_sql_field("p.pid", $value, 1);
        break;
      case "text":
        $comments = 1;
        $sql[] = "(". project_issue_query_sql_field("n.title", $value, 1) ." OR ". project_issue_query_sql_field("n.body", $value, 1) ." OR ". project_issue_query_sql_field("c.body", $value, 1) .")";
        break;
      case "summary":
        $sql[] = "(". project_issue_query_sql_field("n.title", $value, 1) ." OR ". project_issue_query_sql_field("n.body", $value, 1) .")";
        break;
      case "comment":
        $comments = 1;
        $sql[] = project_issue_query_sql_field("c.body", $value, 1);
        break;
      case "categories":
        $sql[] = project_issue_query_sql_field("p.category", $value);
        break;
      case "components":
        $sql[] = project_issue_query_sql_field("p.component", $value);
        break;
      case "versions":
        $sql[] = project_issue_query_sql_field("p.rid", $value);
        break;
      case "states":
        $sql[] = project_issue_query_sql_field("p.state", $value);
        break;
      case "priorities":
        $sql[] = project_issue_query_sql_field("p.priority", $value);
        break;
      case "users":
        $sql[] = "(". project_issue_query_sql_field("n.uid", $value) ." OR ". project_issue_query_sql_field("p.assigned", $value) .")";
        break;
      case "assigned":
        $sql[] = project_issue_query_sql_field("p.assigned", $value);
        break;
      case "submitted":
        $sql[] = project_issue_query_sql_field("n.uid", $value);
        break;
    }
  }

  if (!$comments) { // Don't triple join if not necessary, its rather expensive
    return array("sql" => "SELECT n.nid FROM {project_issues} p LEFT JOIN {node} n ON p.nid = n.nid WHERE n.status = 1 AND (". implode(" AND ", $sql) .")");
  }
  else {
    return array(
      "sql" => "SELECT DISTINCT(n.nid) FROM {project_comments} c LEFT JOIN {project_issues} p ON c.nid = p.nid LEFT JOIN {node} n ON p.nid = n.nid WHERE n.status = 1 AND (". implode(" AND ", $sql) .")",
      "count" => "SELECT COUNT(DISTINCT(n.nid)) FROM {project_comments} c LEFT JOIN {project_issues} p ON c.nid = p.nid LEFT JOIN {node} n ON p.nid = n.nid WHERE n.status = 1 AND (". implode(" AND ", $sql) .")"
    );
  }
}

function project_issue_query_rss($projects, $result) {
  global $base_url, $languages;

  if (db_num_rows($result)) {
    while ($node = db_fetch_object($result)) {
      $node = node_load($node);

      $body = $node->teaser ? $node->teaser : $node->body;

      $items .= format_rss_item(
        $node->title,
        url("node/view/$node->nid"),
        $body,
        array(
          "pubDate" => date("r", ($node->updated > $node->created) ? $node->updated : $node->created),
          "comments" => url("node/view/$node->nid"),
          "category" => $projects[$node->pid]
        )
      );
    }
  }
  else {
    $rows[] = array(array("data" => t("No issues found."), "colspan"=> 8));
  }

  $output .= "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
  $output .= "<!DOCTYPE rss [<!ENTITY % HTMLlat1 PUBLIC \"-//W3C//ENTITIES Latin 1 for XHTML//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml-lat1.ent\">]>\n";
  if (!$channel["version"]) $channel["version"] = "2.0";
  if (!$channel["title"]) $channel["title"] = variable_get("site_name", "drupal") ." - ". t("issues");
  if (!$channel["link"]) $channel["link"] = $base_url;
  if (!$channel["description"]) $channel["description"] = variable_get("site_mission", "");
  foreach ($languages as $key => $value) break;
  if (!$channel["language"]) $channel["language"] = $key ? $key : "en";
  $output .= "<rss version=\"". $channel["version"] . "\">\n";
  $output .= format_rss_channel($channel["title"], $channel["link"], $channel["description"], $items, $channel["language"]);
  $output .= "</rss>\n";

  return $output;
}

?>