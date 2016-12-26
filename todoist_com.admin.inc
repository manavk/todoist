<?php

/**
 *todoist configuration callback function
 */
function _todoist_config_form($form, &$form_state) {
  $form = array();

  $form['user_mail'] = array(
    '#type' => 'textfield',
    '#description' => t('Add Todoist user mail-id'),
    '#default_value' => variable_get('todoist_userid'),
    '#required' => TRUE,
  );

  $form['user_pass'] = array(
    '#type' => 'textfield',
    '#description' => t('Add Todoist user password'),
    '#default_value' => variable_get('user_pass'),
    '#required' => TRUE,
  );

  $form['user_token'] = array(
    '#type' => 'textfield',
    '#description' => t('Add Todoist user token'),
    '#default_value' => variable_get('user_token'),
    '#required' => TRUE,
  );

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
  );

  return $form;
}

/**
 *Submit handler for todoist_config_form
 */
function _todoist_config_form_submit($form, $form_state) {

  if ($form_state['values']['user_mail'] && $form_state['values']['user_pass']) {
    if (!valid_email_address($form_state['values']['user_mail'])) {
      form_set_error('submitted][user_mail', t('The email address appears to be invalid.'));
    }
    else {
      variable_set('todoist_userid', $form_state['values']['user_mail']);
      variable_set('user_pass', $form_state['values']['user_pass']);
      variable_set('user_token', $form_state['values']['user_token']);
      drupal_set_message(t('Your configuration has been saved.'));
      $form_state['rebuild'] = TRUE;
    }
  }
}

