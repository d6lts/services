<?php
// $Id$
/**
 * @file
 * The file contains code which is used to create the services testing interface
 */
/**
 * Callback for 'services/browse'.
 */
function services_admin_browse_index() {
  $methods = services_get_all();
  
  // Show enable server modules.
  $servers = module_implements('server_info');
  $output .= '<h2>'. t('Servers') .'</h2>';
  
  if (!empty($servers)) {
    $output .= '<ul>';
    foreach ($servers as $module) {
      $info = module_invoke($module, 'server_info');
      $name = $info['#name'];
      $path = 'services/'. $info['#path'];
      $output .= '<li class="leaf">'. l($name ." - /$path", $path) .'</li>';
    }
    $output .= '</ul>';
  }
  else {
    $output .= '<p>'. t('You must enable at least one server module to be able to connect remotely. Visit the <a href="@url">modules</a> page to enable server modules.', array('@url' => url('admin/build/modules'))) .'</p>';
  }
  
  
  $output .= '<h2>'. t('Services') .'</h2>';
  
  // group namespaces
  $services = array();
  foreach ($methods as $method) {
    $namespace = drupal_substr($method['#method'], 0, strrpos($method['#method'], '.'));
    $services[$namespace][] = $method;
  }
  
  if (count($services)) {
    foreach ($services as $namespace => $methods) {
      $output .= '<h3>'. $namespace .'</h3>';
      $output .= '<ul>';
      foreach ($methods as $method) {
        $output .= '<li class="leaf">'. l($method['#method'], 'admin/build/services/browse/'. $method['#method']) .'</li>';
      }
      $output .= '</ul>';
    }
  }
  else {
    $output .= t('No services have been enabled.');
  }
  
  return $output;
}

function services_admin_browse_method() {
  global $_services_admin_browse_test_submit_result;
  
  if ($_SERVER['HTTP_X_REQUESTED_WITH'] == 'XMLHttpRequest' && user_access('administer services')) {
    drupal_get_form('services_admin_browse_test');
    $output .= '<h3>'. t('Result') .'</h3>';
    $output .= '<code>'. $_services_admin_browse_test_submit_result .'</code>';
    print $output;
    exit; 
  }
  
  $method = services_method_get(arg(4));
  
  if (!$method) {
    return $output;
  }
  
  //die(print_r($method, true));
  //drupal_set_title($method['#method']);
  $output .= '<h3>'. $method['#method'] .'</h3>';
  $output .= '<p>'. $method['#help'] .'</p>';
  
  // List arguments.
  $output .= '<h3>'. t('Arguments') .' ('. count($method['#args']) .')</h3>';
  $output .= '<dl id="service-browser-arguments">';
  $count = 0;
  foreach ($method['#args'] as $arg) {
    $count++;
    $output .= '<dt><em class="type">'. $arg['#type'] .'</em> <strong class="name">'. $arg['#name'] .'</strong>  ('. (($arg['#optional']) ? t('optional') : t('required')) .')</dt>';
    $output .= '<dd>'. $arg['#description'] .'</dd>';
  }
  
  $output .= '</dl>';
  
  // Allow testing of methods.
  $output .= '<h3>'. t('Call method') .'</h3>';
  $output .= drupal_get_form('services_admin_browse_test');
  
  
  
  // Display results.
  if (user_access('administer services')) {
    $output .= '<div id="output">';
    if ($_services_admin_browse_test_submit_result) {
      $output .= '<h3>'. t('Result') .'</h3>';
      $output .= '<code>'. $_services_admin_browse_test_submit_result .'</code>';
    }
    $output .= '</div>';
  }
  

  return $output;
}

function services_admin_browse_test() {
  $form = array();
  $method = services_method_get(arg(4));
  
  $form['arg'] = array('#tree' => TRUE);
  
  $timestamp = time();
  $nonce = user_password();
  $args_hash = array();
  foreach ($method['#args'] as $key => $arg) {
    $form['name'][$key]         = array('#value' => $arg['#name']);
    $form['optional'][$key]     = array('#value' => ($arg['#optional']) ? t('optional') : t('required'));
    //$form['type'][$key]         = array('#value' => $arg['#type']);
    
    switch ($arg['#name']) {
 
      case 'hash':
        $form['arg'][$key] = array('#title' => 'Hash', '#type' => 'textfield', '#default_value' => hash_hmac("sha256", $timestamp . $_SERVER['HTTP_HOST'] . $nonce . arg(4), services_admin_browse_get_first_key()));
        break;
      
      case 'sessid':
        $form['arg'][$key] = array('#title' => 'Session id', '#type' => 'textfield', '#default_value' => session_id());
        break;
      
      case 'domain_name':
        $form['arg'][$key] = array('#title' => 'Domain name','#type' => 'textfield', '#default_value' => $_SERVER['HTTP_HOST']);
        break;
        
      case 'domain_time_stamp':
        $form['arg'][$key] = array('#title' => 'Timestamp', '#type' => 'textfield', '#default_value' => $timestamp);
        break;
        
      case 'nonce':
        $form['arg'][$key] = array('#title' => 'Nonce', '#type' => 'textfield', '#default_value' => $nonce);
        break;
          
      default:
        $form['arg'][$key] = array('#type' => 'textfield');
        break;
    }
    
  }
    
  $form['submit'] = array('#type' => 'submit', '#value' => t('Call method'));
  
  $form['#redirect'] = FALSE;
  return $form;
}

function services_admin_browse_get_first_key() {
  $keys = services_get_keys();
  foreach ($keys as $kid => $key) {
    return $kid;
  }
}

function services_admin_browse_test_submit($form_id, $form_values) {
  global $_services_admin_browse_test_submit_result;
  $method = services_method_get(arg(4));
  $args = services_admin_browse_test_unserialize_args($form_values['arg']);
  $result = services_method_call($method['#method'], $args);
  $_services_admin_browse_test_submit_result = '<pre>'. htmlspecialchars(print_r($result, TRUE)) .'</pre>';
}

function services_admin_browse_test_unserialize_args($values) {
  $method = services_method_get(arg(4));
  
  // Convert args.
  for ($c = count($method['#args']) - 1; $c >= 0; $c--) {
    $arg = $method['#args'][$c];
    $value = $values[$c];
    
    // Remove empty values from end of array.
    // Once we find a value, we can no longer skip.
    if (!is_numeric($value) && empty($value) && !$noskip) continue;
    $noskip = TRUE;
    
    switch ($arg['#type']) {
      case 'array' :
        if (empty($value)) {
          $return[$c] = NULL;
        }
        else {
          $return[$c] = explode(',', $value);
        }
        
        break;
      default :
        $return[$c] = $value;
    }
  }
  
  if ($return) ksort($return);
  
  //die(print_r($return, true));
  
  return $return;
}

function theme_services_admin_browse_test($form) {
  //die(print_r($form, true));
  $output = '';
  $output .= drupal_render($form['test']);

  $header = array(t('Name'), t('Required'), t('Value'));
  $rows = array();
  foreach (element_children($form['name']) as $key => $type) {
   
    $row = array();
    
    if (isset($form['arg'][$key]['#title'])) {
      $row[] = $form['arg'][$key]['#title'];
      unset($form['arg'][$key]['#title']);
      unset($form['name'][$key]); 
    }
    else{
      $row[] = drupal_render($form['name'][$key]);
    }
    $row[] = drupal_render($form['optional'][$key]);
    $row[] = drupal_render($form['arg'][$key]);
    $rows[] = $row;
  }
  
  $output .= theme('table', $header, $rows);
  $output .= drupal_render($form['submit']);
  
  $output .= drupal_render($form);
  
  return $output;
}
