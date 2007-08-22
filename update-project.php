<?php
require_once "includes/common.inc";

define("PROJECT_NOMAIL", 1);
set_time_limit(0);
theme("header");

$result = db_query("SELECT * FROM {projects} ORDER BY name");
while ($project = db_fetch_object($result)) {
  unset($node);
  $node->type = "project_project";
  $node->uid = $user->uid;
  $node->title = $project->name;
  $node->components = explode(",", $project->areas);
  $node->mail = $project->mail;
  $node->mail_copy = $project->mail;
  $node->mail_digest = $project->mail;
  $node->issues = 1;
  $node->uri = str_replace(" ", "_", strtolower($node->title));
  $node->path = "project/$node->uri";

  $data->nid = node_save($node);
  $node = node_load($data);
  print "<h1>$node->title</h1>";
  print "project converted<br />";

  $releases = array();
  foreach (explode(",", $project->versions) as $version) {
    if (!empty($version)) {
      $releases[$version]->nid = $node->nid;
      $releases[$version]->version = $version;
      $releases[$version]->rid = project_release_save($releases[$version]);
      print "..release $version converted<br />";

      if ($version == $project->version_default) {
        $node->version = $releases[$version]->rid;
        node_save($node);
      }
    }
  }

  $nodes = db_query("SELECT * FROM {node} n LEFT JOIN {project} p USING (nid) WHERE n.type = 'project' AND p.pid = %d", $project->pid);
  while ($entry = db_fetch_object($nodes)) {
    $revisions = unserialize($entry->revisions);
    unset($entry->revisions);
    $revisions[]["node"] = $entry;

    $issue = array_shift($revisions);;
    $issue = $issue["node"];
    $issue->type = "project_issue";
    $issue->pid = $node->nid;
    $issue->category = $issue->ptype;
    $issue->state = $issue->pstatus;
    $issue->component = $issue->area;
    $issue->rid = $releases[$issue->version]->rid;
    //unset($issue->ptype, $issue->pstatus, $issue->area, $issue->version, $issue->file);
    $issue->nid = node_save($issue);
    project_issue_insert($issue);
    db_query("UPDATE {node} SET changed = %d WHERE nid = %d", $issue->changed, $issue->nid);
    print "issue converted: ". l("$issue->title", "node/view/$issue->nid") ."<br />\n";

    foreach ($revisions as $revision) {
      $comment = $revision["node"];
      $comment->type = "project_issue";
      $comment->nid = $issue->nid;
      $comment->type = "project_issue";
      $comment->pid = $node->nid;
      $comment->category = $comment->ptype;
      $comment->state = $comment->pstatus;
      $comment->component = $comment->area;
      $comment->rid = $releases[$comment->version]->rid;
      //unset($comment->ptype, $comment->pstatus, $comment->area, $comment->version, $comment->file);
      $comment->cid = project_comment_save($comment);
      db_query("UPDATE {project_comments} SET created = %d, changed = %d WHERE cid = %d", $comment->changed, $comment->changed, $comment->cid);
      print "..comment converted: $entry->title<br />";
    }
    db_query("UPDATE {node} SET revisions = '', changed = %d WHERE nid = %d", ($comment->changed) ? $comment->changed : $node->created, $issue->nid);
    flush();
  }
}

theme("footer");
?>
