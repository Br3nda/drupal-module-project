<?php
// $Id: mail.inc,v 1.22 2003/12/06 18:39:55 kjartan Exp $

function project_mailhandler($node, $result, $i, $header, $mailbox) {
  if ($node->type == 'project') {
    if (user_access('maintain project issues')) {
      $node->nid = preg_replace('/@.+/', '', $node->nid);

      if ($node->nid) {
        /*
        ** Base the new entry on the node it belongs to, this ensures all
        ** values are initially correct.
        */
        $entry = node_load(array('nid' => $node->nid, 'type' => 'project_issue'));
      }

      // Possible attributes
      $fields = array(
        'pid' => t('Project'),
        'category' => t('Category'),
        'component' => t('Component'),
        'priority' => t('Priority'),
        'rid' => t('Version'),
        'assigned' => t('Assigned to'),
        'state' => t('Status')
      );

      /*
      ** Only change the title if it doesn't have the old title in it.
      ** This should prevent the title from changing due to added
      ** prefixes. It may on occasion make false positives, but if
      ** a title change is that minor who cares?
      */
      $entry->title = (strpos($node->title, $entry->title)) ? $entry->title : $node->title;

      $entry->teaser = $node->teaser;
      $entry->body = $node->body;
      $entry->uid = $node->uid;

      foreach ($fields as $var => $text) {
        $text = strtolower(str_replace(' ', '_', $text));
        if (isset($node->$text)) {
          $node->$text = trim($node->$text);
          switch ($var) {
            case 'pid':
              $project = node_load(array('title' => $node->$text, 'type' => 'project_project'));
              if ($project->nid) {
                $entry->pid = $project->nid;
              }
              break;
            case 'category':
              if (($category = array_search($node->$text, project_issue_category(0, 0)))) {
                $entry->category = $category;
              }
              break;
            case 'priority':
              if (($priority = array_search($node->$text, project_issue_priority()))) {
                $entry->priority = $priority;
              }
              break;
            case 'rid':
              if ($entry->pid && ($rid = db_result(db_query("SELECT rid FROM {project_releases} WHERE nid = %d AND version = '%s'", $entry->pid, $node->$text), 0))) {
                $entry->rid = $rid;
              }
              break;
            case 'assigned':
              if ($user = user_load(array('name' => $node->$text))) {
                $entry->assigned = $user->uid;
              }
              break;
            case 'state':
              if (($state = array_search($node->$text, project_issue_state()))) {
                $entry->state = $state;
              }
              break;
            case 'component':
              if ($project->pid || ($entry->pid && ($project = node_load(array('nid' => $entry->pid, 'type' => 'project_project'))))) {
                if ($project && in_array($node->$text, $project->components)) {
                  $entry->component = $node->$text;
                }
              }
              break;
          }
        }
      }

      if (empty($entry->nid)) {
        $entry->state = 1;
        $entry->type = 'project_issue';
        $entry = node_validate($entry, $error);
        $error or ($entry->nid = node_save($entry));
      }
      else {
        $error = project_comment_validate($entry);
        $error or project_comment_save($entry);
      }
    }
    else {
      $error['user'] = message_access();
    }

    if ($error && $mailbox['replies']) {
      // Send the user his errors
      $mailto = mailhandler_get_fromaddress($header, $mailbox);
      $mailfrom = variable_get('site_mail', ini_get('sendmail_from'));
      $headers = "From: $mailfrom\nReply-to: $mailfrom\nX-Mailer: Drupal Project module (http://drupal.org/project/project)\nReturn-path: $mailfrom\nErrors-to: $mailfrom";

      $body = t('You had some errors in your submission:');
      foreach ($error as $field => $text) {
        $body .= "\n * $field: $text";
      }

      user_mail($mailto, t('Email submission to %sn failed - %subj', array ('%sn' => variable_get('site_name', 'Drupal'), '%subj' => $header->subject)), $body, $headers);
    }

    // Return a NULL result so mailhandler doesn't save the node using the default methods.
    return NULL;
  }
}

function project_mail_urls($url = 0) {
  static $urls = array();
  if ($url) {
    $urls[] = strpos($url, '://') ? $url : url($url);
    return count($urls);
  }
  return $urls;
}

function project_mail_output(&$body, $wrap = 1) {
  static $i = 0;

  $trans = get_html_translation_table(HTML_ENTITIES);
  $trans = array_flip($trans);
  if ($wrap) {
    $pattern = '@(<a href="(.+?)">(.+?)</a>)@ei';
    $body = preg_replace($pattern, "'\\3 ['. project_mail_urls('\\2') .']'", $body);
    $urls = project_mail_urls();
    if (count($urls)) {
      $body .= "\n";
      for ($max = count($urls); $i < $max; $i++) {
        $body .= '['. ($i + 1) .'] '. $urls[$i] ."\n";
      }
    }

    //$body = check_output($body);
    $body = strip_tags($body);
    $body = strtr($body, $trans);
    $body = wordwrap($body, 72);
  }
  else {
    $body = strtr($body, $trans);
  }
}

function project_mail_summary($field, $value) {
  switch ($field) {
    case 'pid':
      $project = node_load(array('nid' => $value, 'type' => 'project_project'));
      return $project->title;
    case 'category':
      return $value ? project_issue_category($value) : t('<none>');
    case 'priority':
      return $value ? project_issue_priority($value) : t('<none>');
    case 'rid':
      if ($value) {
        $release = project_release_load($value);
        return $release->version;
      }
      return t('<none>');
    case 'assigned':
      $user = user_load(array('uid' => $value));
      return $user->name;
    case 'state':
      return $value ? project_issue_state($value) : t('<none>');
    default:
      return $value;
  }
}

function project_mail_notify($node) {
  if (defined('PROJECT_NOMAIL')) {
    return;
  }
  $node = node_load(array('nid' => $node->nid, 'type' => 'project_issue'));
  $project = node_load(array('nid' => $node->pid, 'type' => 'project_project'));
  $fields = array(
    'pid' => t('Project'),
    'rid' => t('Version'),
    'component' => t('Component'),
    'category' => t('Category'),
    'priority' => t('Priority'),
    'assigned' => t('Assigned to'),
    'name' => t('Reported by'),
    'updator' => t('Updated by'),
    'state' => t('Status')
  );

  // Create complete history of the bug report
  $history = array($node);
  $result = db_query('SELECT * FROM {project_comments} WHERE nid = %d ORDER BY changed', $node->nid);
  if (db_num_rows($result)) {
    while ($comment = db_fetch_object($result)) {
      $no_content = empty($comment->body);
      if (!$no_content) {
        $history[] = project_comment_load($comment->cid);
      }
    }
    if ($no_content) {
      return; // No body to notify about.
    }
  }

  // Get most recent update
  $entry = array_pop($history);
  $node->updator = $entry->name ? $entry->name : $user->name;
  $data = unserialize($entry->data);

  // Record users that are connected to this node
  $uids = array();
  if ($entry->uid) {
    $uids[$entry->uid] = "u.uid = $entry->uid";
  }
  if ($node->assigned) {
    $uids[$node->assigned] = "u.uid = $node->assigned";
  }

  // Mail summary (status values)
  foreach ($fields as $field => $text) {
    $text = str_pad("$text:", 14);
    if (isset($data['new']->$field)) {
      $summary .= "-$text". project_mail_summary($field, $data['old']->$field) ."\n";
      $summary .= "+$text". project_mail_summary($field, $data['new']->$field) ."\n";
    }
    else {
      $summary .= " $text". project_mail_summary($field, $node->$field) ."\n";
    }
  }

  // Create main body content
  project_mail_output($entry->body);
  $body = "$entry->body\n\n$entry->name\n";

  $hr = str_repeat('-', 72);

  if (count($history)) {
    $body .= "\n\n\n";
    $body .= t('Previous comments:');
    foreach ($history as $entry) {
      if ($entry->uid) {
        $uids[$entry->uid] = "u.uid = $entry->uid";
      }
      project_mail_output($entry->body);
      $body .= "\n$hr\n\n";
      $body .= format_date($entry->created) ." : $entry->name\n\n";
      $body .= "$entry->body\n";
    }
  }

  // Create the footer
  $footer  = "-- \n";
  $footer .= t('View: %link', array('%link' => url("node/view/$node->nid"))) ."\n";
  $footer .= t('Edit: %link', array('%link' => url("project/comments/add/$node->nid"))) ."\n";

  $body = "$summary\n$body\n$footer\n";

  // Create mail header
  $sender->name = user_mail_encode(($pos = strpos($node->updator, '@')) ? substr($node->updator, 0, $pos) : $node->updator);
  $sender->mail = $project->mail;
  $domain = preg_replace('|.+://([a-zA-Z0-9\._-]+).*|', '\1', $GLOBALS['base_url']);
  $header = array(
    "Return-Path: <$sender->mail>",
    'Date: '. date('r'),
    "From: $sender->name <$sender->mail>",
    'X-Mailer: Drupal Project module (http://drupal.org/project/project)',
    "List-Id: $project->title <$project->uri-issues-$domain>",
    'List-Archive: <'. url("project/$project->uri/issues") .'>',
    'List-Subscribe: <'. url("node/view/$project->uri") .'>',
    'List-Unsubscribe: <'. url("node/view/$project->uri") .'>'
  );
  if ($replyto = variable_get('project_reply_to', '')) {
    $header[] = "Reply-To: $project->title <$replyto>";
  }

  if (count($history)) {
    $header[] = "Message-Id: <type=project&nid=$node->nid&cid=$entry->cid&host=@$domain>";
    $header[] = "In-Reply-To: <type=project&nid=$node->nid&host=@$domain>";
    $header[] = "References: <type=project&nid=$node->nid&host=@$domain> <type=project&nid=$node->nid&revcount=1&host=@$domain>";
  }
  else {
    $header[] = "Message-Id: <type=project&nid=$node->nid&host=@$domain>";
  }

  if (count($uids)) {
    $result = db_query('SELECT p.*, u.name, u.mail FROM {project_subscriptions} p LEFT JOIN {users} u ON p.uid = u.uid WHERE u.status = 1 AND p.nid = %d AND (p.level = 2 OR (p.level = 1 AND (%s)))', $node->pid, implode(' OR ', $uids));
  }
  else {
    $result = db_query('SELECT p.*, u.name, u.mail FROM {project_subscriptions} p LEFT JOIN {users} u ON p.uid = u.uid WHERE u.status = 1 AND p.nid = %d AND p.level = 2', $node->pid);
  }

  project_mail_output($node->title, 0);
  $node->title = t("[$node->category]") ." $node->title";
  while ($recipient = db_fetch_object($result)) {
    user_mail($recipient->mail, $node->title, $body, implode("\n", $header));
  }

  if ($project->mail_copy && (!count($project->mail_copy_filter) || $project->mail_copy_filter[$node->category])) {
    user_mail($project->mail_copy, $node->title, $body, implode("\n", $header));
  }
}

function project_mail_reminder() {
  $projects = array();
  $result = db_query('SELECT n.nid, n.title FROM {project_projects} p LEFT JOIN {node} n USING (nid) WHERE p.mail_reminder = 1 AND n.status = 1');
  while ($project = db_fetch_object($result)) {
    $projects[$project->nid] = $project->title;
    $pids[] = "p.pid = $project->nid";
  }
  if (count($projects)) {

    $sender->name = variable_get('site_name', '');
    $sender->mail = variable_get('site_mail', '');

    $result = db_query('SELECT n.*, p.*, u.name, u.mail FROM {project_issues} p LEFT JOIN {node} n ON p.nid = n.nid LEFT JOIN {users} u ON n.uid = u.uid WHERE n.status = 1 AND u.status = 1 AND ('. implode(' OR ', $pids) .") AND u.mail <> '' AND (p.state = 1 OR p.state = 2) ORDER BY u.uid, p.pid, p.component, p.state, n.changed DESC");

    while (($node = db_fetch_object($result)) || !empty($body)) {
      if ($body && $mail && ((!$node) || ($mail != $node->mail))) {
        $header = array(
          "Return-Path: <$sender->mail;>",
          'Date: '. date('r'),
          "From: $sender->name <$sender->mail>",
          'X-Mailer: Drupal Project module (http://drupal.org/project/project)',
          "List-Id: $sender->name <project-reminder-". preg_replace('|.+://([a-zA-Z0-9\._-]+).*|', '\1', $GLOBALS['base_url']) .'>',
          'List-Archive: <'. url('project') .'>',
        );

        user_mail($mail, t('Your submitted bugs for %date', array('%date' => date('F d, Y', time()))), $body, implode("\n", $header));

        $body = '';
      }
      $mail = $node->mail;

      if ($node->nid) {
        if ($pid != $node->pid || $component != $node->component) {
          $pid = $node->pid;
          $component = $node->component;
          $banner = "$projects[$pid] / $component";
          $body .= "[ $banner ]". str_repeat('=', 72 - 4 - strlen($banner)) ."\n";
        }
        $body .= "$node->title\n";
        if ($node->assigned && $assigned = user_load(array('uid' => $node->assigned))) {
          $body .= "  assigned: $assigned->name\n";
        }
        $body .= '  state: '. project_issue_state($node->state) ."\n";
        $body .= '  age: '. format_interval(time() - $node->created) ."\n";
        $body .= '  url: '. url("node/view/$node->nid") ."\n";
        $body .= "\n";
      }
    }
  }
}

function project_mail_digest() {
  $header = array(
    'Date: '. date('r'),
    'X-Mailer: Drupal Project module (http://drupal.org/project/project)',
    "List-Id: $project->title <". preg_replace('|.+://([a-zA-Z0-9\._-]+).*|', '\1', $GLOBALS['base_url']) .'-project-issues-digest>',
    'List-Archive: <'. url('project/issues?priorities=1') .'>',
  );

  $result = db_query("SELECT * FROM {project_projects} p LEFT JOIN {node} n USING (nid) WHERE n.status = 1 AND p.mail_digest <> '' ORDER BY title, p.mail_digest");
  while ($project = db_fetch_object($result)) {
    $category = '';
    $result2 = db_query('SELECT n.nid, n.title, n.created, p.state, p.category, p.component, p.priority, p.assigned FROM {project_issues} p LEFT JOIN {node} n ON p.nid = n.nid WHERE n.status = 1 AND p.pid = %d AND p.state = 1 AND p.priority = 1 ORDER BY p.category, n.created DESC', $project->nid);
    while ($node = db_fetch_object($result2)) {
      if ($category != $node->category) {
        $category = $node->category;
        $banner = "$project->title / ". project_issue_category($node->category);
        $body[$project->mail_digest] .= "$banner\n". str_repeat('-', $banner) ."\n";
      }
      $body[$project->mail_digest] .= "$node->title\n";
      if ($node->assigned && $assigned = user_load(array('uid' => $node->assigned))) {
        $body[$project->mail_digest] .= '  assigned: '. $assigned->name ."\n";
      }
      $body[$project->mail_digest] .= '  age: '. format_interval(time() - $node->created) ."\n";
      $body[$project->mail_digest] .= '  url: '. url("node/view/$node->nid") ."\n";
      $body[$project->mail_digest] .= "\n";
    }
  }

  foreach ($body as $mailto => $text) {
    $header['from'] = "From: $mailto";
    $header['return-path'] = "Return-Path: <$mailto>";

    user_mail($mailto, t('Release critical bugs for %date', array('%date' => date('F d, Y', time()))), $text, implode("\n", $header));
  }
}

?>