package org.apache.http.client;

import org.apache.http.HttpRequest;

/**
 * @hide
 */
 
public interface HttpRequestCheckHandler {
    
    public boolean checkRequest(HttpRequest request);

}