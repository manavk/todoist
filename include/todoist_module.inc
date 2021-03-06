<?php

/**
 *todoist content callback function
 */
function _todoist_content_list() {

  if (!variable_get('user_token')) {
    global $base_url;
    return drupal_set_message(t('There is an error in your configuration settings please check your ToDoist Api setting under @url', array('@url' => $base_url . '/admin/config/user-interface/todoist')), 'warning');
  }
  else {
    // get api token
    $token = variable_get('user_token');
    $user_apicall = 'https://todoist.com/API/v7/sync?';
    $user_apicall .= 'token=' . $token;
    $user_check = drupal_http_request($user_apicall);

    if ($user_check->status_message != 'OK') {
      drupal_set_message(t('There is an error in your configuration settings please check your ToDoist Api token, user mail or password setting under @url', array('@url' => $base_url . '/admin/config/user-interface/todoist')), 'warning');
      return 'Error';
    }
    else {
      $apicall = 'https://todoist.com/API/v7/sync?';
      $apicall .= 'token=' . $token;
      $apicall .= '&sync_token=*&resource_types=["all"]';
      $http_result = drupal_http_request($apicall);
      $data = $http_result->data;
      $data = drupal_json_decode($data);
      $items = isset($data['items']) ? $data['items'] : '';
      $header = array(
        array('data' => 'Task', 'sort' => 'asc'),
        array('data' => 'Task added date'),
        array('data' => 'Task due date'),
        array('data' => 'Action', 'colspan' => 3),
      );
      $rows = array();
      if ($items) {
        foreach ($items as $node) {
          $duedate = end($node);
          $rows[] = array(
            $node['content'],
            isset($node['date_added']) ? date('D, j M Y', strtotime($node['date_added'])) : 'No date',
            !empty($node['date_string']) ? $node['date_string'] : 'No due date',
            l(t('Complete'), 'todoist/' . $node['id'] . '/complete'),
            l(t('Edit'), 'todoist/' . $node['id'] . '/edit/'),
            l(t('Delete'), 'todoist/' . $node['id'] . '/delete'),
          );
        }
      }

      $html = theme('table',
        array(
          'header' => $header,
          'rows' => $rows,
          // Optional to indicate whether the table headers should be sticky
          'sticky' => TRUE,
          // Optional empty text for the table if resultset is empty
          'empty' => 'No task created...',
        )
      );

      return $html;
    }
  }
}

/**
 *todoist task add callback function
 */
function _todoist_task_add_form($form, $form_state) {

  $form = array();

  $form['task_content'] = array(
    '#title' => 'Task',
    '#type' => 'textfield',
    '#description' => t('Add task heading'),
    '#required' => TRUE,
  );

  $form['task_due_data'] = array(
    '#title' => 'Due data',
    '#type' => 'textfield',
    '#description' => t('Add due data in mm/dd/yy format e.g fri at 2pm, today, today at 2, 10/5, 10/5/2006 at 2pm, 5 10 2010 etc. for more info please check this link <a href=
    "https://support.todoist.com/hc/en-us/articles/205325931-Dates-and-Times">Todoist datatime</a>'),
    '#required' => TRUE,
  );

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
  );

  $form['cancel'] = array(
    '#type' => 'submit',
    '#value' => t('Cancel'),
  );

  return $form;
}

/**
 *Submit handler for _todoist_task_add_form
 */
function _todoist_task_add_form_submit($form, &$form_state) {
  switch ($form_state['values']['op']) {
    case 'Cancel':
      $form_state['redirect'] = 'admin/content/todoist';
      break;

    case 'Save':
      if ($form_state['values']['task_content'] && $form_state['values']['task_due_data']) {

        $content = $form_state['values']['task_content'];
        $date = $form_state['values']['task_due_data'];

        $apicall = 'https://todoist.com/API/v7/items/add?';

        $data = array(
          // get api token
          'token' => variable_get('user_token'),
          'content' => $content,
          'date_string' => $date,
        );

        $api = url($apicall, array('query' => $data));
        $http_result = drupal_http_request($api);

        if ($http_result->status_message == 'OK') {
          drupal_set_message(t('Task successfully added.'), 'status');
          $form_state['redirect'] = 'admin/content/todoist';
        }
        else {
          drupal_set_message(t('Please check your data or configuration setting.', 'warning'));
        }
      }
      break;
  }
}

/**
 *todoist task delete callback function
 */
function _todoist_task_delete_form($form, &$form_state, $task_id) {

  $form = array();

  $form['task_id'] = array(
    '#type' => 'value',
    '#value' => $task_id,
  );

  //confirm message
  return confirm_form(
    $form,
    t('Are you sure you want to delete this task?'),
    'admin/content/todoist/',
    t('This action cannot be undone.'),
    t('Delete'),
    t('Cancel')
  );

  return $form;
}

/**
 *Submit handler for _todoist_task_delete_form
 */
function _todoist_task_delete_form_submit($form, &$form_state) {

  if ($form_state['values']['task_id']) {
    $ids = $form_state['values']['task_id'];
    $token = variable_get('user_token');
    $apicall = 'https://todoist.com/API/v7/sync?';
    $post_data = array(
      'token' => $token,
      'commands' => '[{"type": "item_delete", "uuid": "f8539c77-7fd7-4846-afad-3b201f0be8a5", "args": {"ids": [' . $ids . ']}}]',
    );
    $ch = curl_init();
    curl_setopt($ch, CURLOPT_URL, $apicall);
    curl_setopt($ch, CURLOPT_RETURNTRANSFER, 1);
    curl_setopt($ch, CURLOPT_POST, 1);
    curl_setopt($ch, CURLOPT_POSTFIELDS, $post_data);
    curl_setopt($ch, CURLOPT_RETURNTRANSFER, TRUE);

    $parsed = curl_exec($ch);
    $result = json_decode($parsed, TRUE);
    if (curl_errno($ch)) {
      drupal_set_message('Error:' . curl_error($ch), 'error');
    }
    else {
      curl_close($ch);
      drupal_set_message('1 task deleted successfully.');
      $form_state['redirect'] = 'admin/content/todoist/';
    }
  }
}

/**
 *todoist task complete callback function
 */
function _todoist_task_complete_form($form, &$form_state, $task_id) {

  $form = array();

  $form['task_id'] = array(
    '#type' => 'value',
    '#value' => $task_id,
  );

  //confirm message
  return confirm_form(
    $form,
    t('Nice, you have completed your task.'),
    'admin/content/todoist/',
    t('This action cannot be undone.'),
    t('Complete'),
    t('Cancel')
  );

  return $form;
}

/**
 *Submit handler for _todoist_task_complete_form
 */
function _todoist_task_complete_form_submit($form, &$form_state) {

  if ($form_state['values']['task_id']) {
    $ids = $form_state['values']['task_id'];
    $token = variable_get('user_token');
    $apicall = 'https://todoist.com/API/v7/sync?';
    $post_data = array(
      'token' => $token,
      'commands' => '[{"type": "item_complete", "uuid": "a74bfb5c-' . GeraHash(4) . '-4d14-baea-' . GeraHash(12) . '", "args": {"ids": [' . $ids . ']}}]',
    );
    $ch = curl_init();
    curl_setopt($ch, CURLOPT_URL, $apicall);
    curl_setopt($ch, CURLOPT_RETURNTRANSFER, 1);
    curl_setopt($ch, CURLOPT_POST, 1);
    curl_setopt($ch, CURLOPT_POSTFIELDS, $post_data);
    curl_setopt($ch, CURLOPT_RETURNTRANSFER, TRUE);

    $parsed = curl_exec($ch);
    $result = json_decode($parsed, TRUE);
    if (curl_errno($ch)) {
      drupal_set_message('Error:' . curl_error($ch), 'error');
    }
    else {
      curl_close($ch);
      drupal_set_message('1 task completed successfully.');
      $form_state['redirect'] = 'admin/content/todoist/';
    }
  }
}

/**
 *todoist task edit callback function
 */
function _todoist_task_edit_form($form, $form_state, $task_id) {

  $ids = $task_id;
  $apicall = 'https://todoist.com/API/v7/items/get?';
  $apicall .= '&token=' . variable_get('user_token');
  $apicall .= '&item_id=' . $ids;

  $http_result = drupal_http_request($apicall);
  if ($http_result->status_message == 'OK') {
    $datas = drupal_json_decode($http_result->data);
    $data = $datas['item'];
  }

  $form = array();

  $form['task_id'] = array(
    '#type' => 'value',
    '#value' => $task_id,
  );

  $form['task_content'] = array(
    '#title' => 'Task',
    '#type' => 'textfield',
    '#default_value' => $data['content'],
    '#description' => t('Add task heading'),
    '#required' => TRUE,
  );

  $form['task_due_data'] = array(
    '#title' => 'Due data',
    '#type' => 'textfield',
    '#default_value' => $data['date_string'],
    '#description' => t('Add due data in mm/dd/yy format e.g fri at 2pm, today, today at 2, 10/5, 10/5/2006 at 2pm, 5 10 2010 etc. for more info please check this link <a href=
    "https://todoist.com/Help/DatesTimes">Todoist datatime</a>'),
    '#required' => TRUE,
  );

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
  );

  $form['cancel'] = array(
    '#type' => 'submit',
    '#value' => t('Cancel'),
  );

  return $form;
}

/**
 *Submit handler for _todoist_task_edit_form
 */
function _todoist_task_edit_form_submit($form, &$form_state) {
  switch ($form_state['values']['op']) {
    case 'Cancel':
      $form_state['redirect'] = 'admin/content/todoist';
      break;

    case 'Save':

      if ($form_state['values']['task_content'] && $form_state['values']['task_due_data']) {

        $content = $form_state['values']['task_content'];
        $date = $form_state['values']['task_due_data'];
        $id = $form_state['values']['task_id'];
        $token = variable_get('user_token');
        $apicall = 'https://todoist.com/API/v7/sync?';
        $post_data = array(
          'token' => $token,
          'commands' => '[{"type": "item_update", "uuid": "318d16a7-0c88-46e0-9eb5-cde6c72477c8", "args": {"id": ' . $id . ', "content": "' . $content . '", "date_string": "' . $date . '"}}]',
        );
        $ch = curl_init();
        curl_setopt($ch, CURLOPT_URL, $apicall);
        curl_setopt($ch, CURLOPT_RETURNTRANSFER, 1);
        curl_setopt($ch, CURLOPT_POST, 1);
        curl_setopt($ch, CURLOPT_POSTFIELDS, $post_data);
        curl_setopt($ch, CURLOPT_RETURNTRANSFER, TRUE);

        $parsed = curl_exec($ch);
        $result = json_decode($parsed, TRUE);
        if (curl_errno($ch)) {
          drupal_set_message('Error:' . curl_error($ch), 'error');
        }
        else {
          curl_close($ch);
          drupal_set_message('1 task successfully edit.');
          $form_state['redirect'] = 'admin/content/todoist/';
        }
      }
      break;
  }
}

//Return random string for UUID edit task
function GeraHash($qtd) {
  //Under the string $Caracteres you write all the characters you want to be used to randomly generate the code.
  $Caracteres = 'abcdefghijklmnopqrstuvwxyz0123456789';
  $QuantidadeCaracteres = strlen($Caracteres);
  $QuantidadeCaracteres--;

  $Hash = NULL;
  for ($x = 1; $x <= $qtd; $x++) {
    $Posicao = rand(0, $QuantidadeCaracteres);
    $Hash .= substr($Caracteres, $Posicao, 1);
  }

  return $Hash;
}

