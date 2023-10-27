import {withAuthMiddleware} from "@use-pico/auth";

export default withAuthMiddleware({
    routes: [
        {
            path: "/games",
        },
        {
            path:   "/root",
            tokens: [
                "root",
            ],
        }
    ],
});
