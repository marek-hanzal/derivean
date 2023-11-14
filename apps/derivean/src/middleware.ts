import {redirects}          from "@/derivean/redirects";
import {withAuthMiddleware} from "@use-pico/auth-server";

export default withAuthMiddleware({
    routes: redirects,
});
