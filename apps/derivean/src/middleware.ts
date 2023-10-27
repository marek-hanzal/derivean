import {withAuthMiddleware} from "@use-pico/auth";

export default withAuthMiddleware({
    routes: [
        {
            path: "/game",
        },
        {
            path:   "/root",
            tokens: [
                "root",
            ],
        }
    ],
});
