<?php

/**
 * @file
 * Contains Drupal\services\ServiceAPIInterface.
 */

namespace Drupal\services;

use Drupal\Core\Config\Entity\ConfigEntityInterface;

/**
 * Provides an interface for defining Service api entities.
 */
interface ServiceAPIInterface extends ConfigEntityInterface {

  /**
   * Returns the endpoint path to the API.
   * @return string
   */
  public function getEndpoint();

  /**
   * Returns the service provider ID.
   * @return string
   */
  public function getServiceProvider();
}
