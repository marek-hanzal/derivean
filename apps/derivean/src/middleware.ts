import {redirects}          from "@/derivean/redirects";
import {withAuthMiddleware} from "@use-pico/server";

export default withAuthMiddleware({
    routes: redirects,
});
