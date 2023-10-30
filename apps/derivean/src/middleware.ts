import {withAuthMiddleware} from "@use-pico2/auth";

export default withAuthMiddleware({
    routes: [
        {
            path: "/game",
            site: "/game",
        },
        {
            path:   "/root",
            site: "/root",
            tokens: [
                "root",
            ],
        }
    ],
});
