import {env}              from "@/derivean/env";
import {container}        from "@derivean/server";
import {withAuthEndpoint} from "@use-pico/auth";
import GitHub             from "next-auth/providers/github";

const endpoint = withAuthEndpoint({
    container,
    providers: [
        GitHub({
            name:         "github",
            clientId:     env.NEXTAUTH_GITHUB_CLIENT_ID,
            clientSecret: env.NEXTAUTH_GITHUB_CLIENT_SECRET,
        }),
    ],
});

export {
    endpoint as GET,
    endpoint as POST
};
