<?php
/* $Id: comment.inc,v 1.34 2004/09/11 11:13:55 kjartan Exp $ */

function project_comment_page() {
  switch ($_POST['op'] ? $_POST['op'] : arg(2)) {
    case 'add':
      if (user_access('create project issues')) {
        $node = node_load(array('nid' => arg(3), 'type' => 'project_issue'));

        foreach (array('nid', 'type', 'pid', 'rid', 'category', 'component', 'priority', 'assigned', 'state', 'title') as $var) {
          $comment->$var = $node->$var;
        }

        $comment = project_comment_validate($comment);
        $output .= form(project_comment_form($comment, $param), ($param['method'] ? $param['method'] : 'post'), $param['action'], $param['options']);
        $output .= node_view($node);

        $breadcrumb = drupal_get_breadcrumb();
        $breadcrumb[] = l($node->title, "node/$node->nid");
        drupal_set_breadcrumb($breadcrumb);
        drupal_set_title(t('New comment'));
        print theme('page', $output);
      }
      break;
    case t('Preview'):
      if (user_access('create project issues')) {
        $edit = array2object($_POST['edit']);
        $edit->type = 'project_issue';
        $edit = project_comment_validate($edit);

        print theme('page', node_preview($edit, $error));
      }
      break;
    case t('Submit'):
      if (user_access('create project issues')) {
        $edit = array2object($_POST['edit']);
        $edit = project_comment_validate($edit);
        if ($error) {
          $edit->type = 'project_issue';
          print theme('page', node_preview($edit, $error));
        }
        else {
          $node = node_load(array('nid' => arg(3), 'type' => 'project_issue'));
          $edit->nid = $node->nid;

          project_comment_save($edit);
          drupal_goto(url("node/$node->nid"));
        }
      }
      break;
  }
}

function project_comment_form(&$edit, &$param) {
  $output = '<div class="node-form">';
  $output .= '<div class="standard">';
  $output .= form_textfield(t('Title'), 'title', $edit->title, 60, 128, NULL, NULL, TRUE);
  $output .= project_issue_form($edit, $param);
  $output .= '</div></div>';

  if ($edit->cid) {
    $output .= form_hidden('cid', $edit->cid);
  }

  $output .= form_submit(t('Preview'));

  if (!form_get_errors()) {
    $output .= form_submit(t('Submit'));
  }

  return $output;
}

function project_comment_validate($edit) {
  global $user;

  $edit->uid = $user->uid;
  $edit->name = $user->name;

  if ($edit->cid) {
    $comment = project_comment_load($edit->cid);
    $edit->nid = $comment->nid;
    $edit->fid = $comment->fid;
  }

  $edit->comment = true;

  $edit = project_issue_validate($edit);
  $edit->validated = true;

  return $edit;
}

function project_comment_view($node, $main = 0) {
  global $user;
  $links = array();
  $rows = array();
  $fields = array(
    'title' => 'Title',
    'pid' => 'Project',
    'rid' => 'Version',
    'component' => 'Component',
    'category' => 'Category',
    'priority' => 'Priority',
    'assigned' => 'Assigned to',
    'state' => 'Status'
  );

  $result = db_query('SELECT p.*, u.name FROM {project_comments} p INNER JOIN {users} u USING (uid) WHERE p.nid = %d ORDER BY p.created ASC', $node->nid);
  if (db_num_rows($result)) {
    $i = 0;
    while ($comment = db_fetch_object($result)) {
      $i++;
      $data = unserialize($comment->data);
      $summary = array();
      foreach ($fields as $field => $text) {
        if (isset($data['old']->$field) && isset($data['new']->$field)) {
          $summary[] = array(
            t($text) .':',
            project_mail_summary($field, $data['old']->$field),
            '&raquo; '. project_mail_summary($field, $data['new']->$field)
          );
        }
      }

      if ($comment->file_path && file_exists($comment->file_path)) {
        $summary[] = array(t('Attachment:'), '<a href="'. file_create_url($comment->file_path). '">'. basename($comment->file_path) .'</a> ('. format_size($comment->file_size) .')');
      }

      if ($summary || $comment->body) {
        $rows[] = array(array('class' => 'header', 'data' => t('%count submitted by %user on %date', array('%count' => l("#$i", "node/$node->nid", array ('id' => "comment-$comment->cid", 'name' => "comment-$comment->cid"), NULL, "comment-$comment->cid"), '%user' => format_name($comment), '%date' => format_date($comment->created))) . (node_is_new($node->nid, $comment->changed) ? theme_mark() : '')));
        if ($summary) {
          $rows[] = array(array('class' => 'summary', 'data' => theme('table', NULL, $summary)));
        }
        if ($comment->body) {
          $rows[] = array(array('class' => 'content', 'data' => '<p>'. check_output($comment->body) .'</p>'));
        }
        if ($comment->fid) {
          $rows[] = array(l(t('download attachment'), "project/comments/download/$comment->cid"));
        }
      }
    }
    $output = '<div class="project">';
    $output .= theme('table', NULL, $rows);
    $output .= '</div>';

    return theme('box', t('Updates'), $output);
  }
}

function project_comment_load($cid) {
  return db_fetch_object(db_query('SELECT p.*, u.name FROM {project_comments} p INNER JOIN {users} u USING (uid) WHERE p.cid = %d ORDER BY p.created DESC', $cid));
}

function project_comment_save($edit) {
  global $user;

  if (empty($edit->cid)) {
    $edit->cid = db_next_id('project_cid');
    if ($edit->file) {
      $file = file_save_upload($edit->file, variable_get('project_directory_issues', 'issues'));
      unset($edit->file);
    }

    if (empty($edit->uid)) {
      $edit->uid = $user->uid;
    }
    $node = node_load(array('nid' => $edit->nid, 'type' => 'project_issue'));

    // Check if comment changed any of the state values and update the node if necessary
    foreach (array('pid', 'rid', 'category', 'component', 'priority', 'assigned', 'state', 'title') as $var) {
      if ($node->$var != $edit->$var) {
        $data['old']->$var = $node->$var;
        $data['new']->$var = $edit->$var;
        $node->$var = $edit->$var;
      }
    }

    watchdog('special', "project: added comment '$edit->title'", l('view post', "node/$node->nid"));
    db_query("INSERT INTO {project_comments} (cid, nid, uid,  created, changed, body, data, file_path, file_mime, file_size) VALUES (%d, %d, %d, %d, %d, '%s', '%s', '%s', '%s', %d)", $edit->cid, $edit->nid, $edit->uid, time(), time(), $edit->body, serialize($data), $file->filepath, $file->filemime, $file->filesize);
    node_save($node);
  }

  return $edit->cid;
}

?>
