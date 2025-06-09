<?php

class SM_ssoLogin extends SM_module {

    function moduleConfig() {
        // no user input, no forms
    }

    function canModuleLoad() {
        return $this->sessionH->directive['useMemberSystem'];
    }

    function moduleThink() {
        // If already logged in, just redirect
        if (!$this->sessionH->isGuest()) {
            header("Location: /");
            return;
        }

        // get username and email from proxy headers
        $user = isset($_SERVER['HTTP_X_FORWARDED_USER']) ? $_SERVER['HTTP_X_FORWARDED_USER'] : null;
        $emailAddress = isset($_SERVER['HTTP_X_FORWARDED_EMAIL']) ? $_SERVER['HTTP_X_FORWARDED_EMAIL'] : null;

        if ($user || $emailAddress) {
            // attempt SSO login using username and email
            if ($this->sessionH->attemptLoginSSO($user, $emailAddress)) {
                // logged in successfully
                $this->say("Successful Login!");
                header("Location: /");
                // $this->sessionH->reloadPage();
            } else {
                // unsuccessful login
                $this->say("Sorry, SSO login was incorrect or user not found.");
            }
        } else {
            // missing headers, show error or fallback
            $this->say("SSO authentication headers missing.");
        }

    }

}
?>
