import {withAuthMiddleware} from "@use-pico2/auth";

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
