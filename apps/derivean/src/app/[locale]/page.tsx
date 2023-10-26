import {CheckAuthenticatedSession} from "@use-pico/auth";
import {LoadingOverlay}            from "@use-pico/ui";

export default function Index() {
    return <>
        <CheckAuthenticatedSession
            redirect={"/game"}
        />
        <LoadingOverlay
            visible
        />
    </>;
}
