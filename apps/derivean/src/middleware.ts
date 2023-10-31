import {withAuthMiddleware} from "@use-pico/auth";

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
