import {withAuthMiddleware} from "@use-pico/auth-server";

export default withAuthMiddleware({
    routes: [
        {
            path:   "/public",
            target: "/-/game",
            auth:   false,
            tokens: [
                "game",
            ]
        },
        {
            path:   "/public",
            target: "/-/manager",
            auth:   false,
            tokens: [
                "manager",
            ]
        },
        {
            path:   "/game",
            target: "/-/public",
            auth:   true,
            tokens: [
                "game",
            ]
        },
        {
            path:   "/root",
            target: "/-/public",
            auth:   true,
            tokens: [
                "root",
            ],
        }
    ],
});
