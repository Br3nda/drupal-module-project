<?php
// $Id: project_release_handler_field_files.inc,v 1.1 2009/08/04 18:03:37 dww Exp $

/**
 * Field handler to provide a list of files associated with a release node.
 *
 * Based heavily on views_handler_field_upload_fid.inc from
 * contributions/modules/views/modules/upload (forked from revision 1.7).
 * However, we're using separate tables ({project_release_file}, not
 * {upload}), we support a slightly different set of tokens, we need different
 * download links, different handler options, etc. So, it wasn't really
 * possible to extend views_handler_field_upload_fid.inc...
 */
class project_release_handler_field_files extends views_handler_field_prerender_list {
  function construct() {
    parent::construct();
  }

  function option_definition() {
    $options = parent::option_definition();
    $options['link_to_file'] = array('default' => FALSE);
    return $options;
  }

  function options_form(&$form, &$form_state) {
    parent::options_form($form, $form_state);
    $form['link_to_file'] = array(
      '#title' => t('Link this field to download the file'),
      '#type' => 'checkbox',
      '#default_value' => !empty($this->options['link_to_file']),
    );
  }

  function pre_render($values) {
    $nids = array();
    $this->items = array();
    foreach ($values as $result) {
      $nids[] = $result->{$this->field_alias};
    }
    if (!empty($nids)) {
      // TODO: better ORDER BY?  Should we have a {prf}.weight field?
      $result = db_query("SELECT prf.nid, prf.fid, prf.filehash, f.uid, f.filename, f.filepath, f.filesize, f.filemime, f.timestamp FROM {project_release_file} prf LEFT JOIN {files} f ON prf.fid = f.fid WHERE prf.nid IN (" . implode(', ', $nids) . ") ORDER BY f.filename");
      while ($file = db_fetch_array($result)) {
        $file['filename'] = check_plain($file['filename']);
        $file['filemime'] = check_plain($file['filemime']);
        $file['filesize'] = format_size($file['filesize']);
        $file['timestamp'] = format_date($file['timestamp']);
        $file_link = theme('project_release_download_link', $file['filepath'], NULL, TRUE);
        $file['filepath'] = $file_link['href'];
        if (!empty($this->options['link_to_file']) ) {
          $file['make_link'] = TRUE;
          $file['path'] = $file['filepath'];
        }
        $this->items[$file['nid']][$file['fid']] = $file;
      }
    }
  }

  function render_item($count, $item) {
    return $item['filename'];
  }

  function document_self_tokens(&$tokens) {
    $tokens['[' . $this->options['id'] . '-fid' . ']'] = t('The file ID for the file.');
    $tokens['[' . $this->options['id'] . '-name' . ']'] = t('The name of the attached file.');
    $tokens['[' . $this->options['id'] . '-ext' . ']'] = t("The filename extenstion (e.g. 'zip' or 'tar.gz') of the attached file.");
    $tokens['[' . $this->options['id'] . '-type' . ']'] = t('The MIME type of the attached file.');
    $tokens['[' . $this->options['id'] . '-path' . ']'] = t('The path of the attached file.');
    $tokens['[' . $this->options['id'] . '-size' . ']'] = t('The size of the attached file.');
    $tokens['[' . $this->options['id'] . '-hash' . ']'] = t('The MD5 hash of the attached file.');
    $tokens['[' . $this->options['id'] . '-time' . ']'] = t('The date and time when the file was created or uploaded.');
  }

  function add_self_tokens(&$tokens, $item) {
    $tokens['[' . $this->options['id'] . '-fid' . ']'] = $item['fid'];
    $tokens['[' . $this->options['id'] . '-name' . ']'] = $item['filename'];
    $tokens['[' . $this->options['id'] . '-type' . ']'] = $item['filemime'];
    $tokens['[' . $this->options['id'] . '-path' . ']'] = $item['filepath'];
    $tokens['[' . $this->options['id'] . '-size' . ']'] = $item['filesize'];
    $tokens['[' . $this->options['id'] . '-hash' . ']'] = $item['filehash'];
    $tokens['[' . $this->options['id'] . '-time' . ']'] = $item['timestamp'];
    $file_parts = explode('.', basename($item['filename']));
    $ext = array_pop($file_parts);
    // Special cases for 'gz' and 'bz2', since you really need to know the
    // thing before that, too (e.g. '.tar.gz').
    if ($ext == 'gz' || $ext == 'bz2') {
      $ext = array_pop($file_parts) . '.' . $ext;
    }
    $tokens['[' . $this->options['id'] . '-ext' . ']'] = $ext;
  }
}

