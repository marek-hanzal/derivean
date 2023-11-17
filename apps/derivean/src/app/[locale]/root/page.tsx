import {Page} from "@use-pico/client";
import {t}    from "@use-pico/translator";

export default function Index() {
    return <Page
        text={{
            header: t()`Low-level system administration`,
        }}
    >
    </Page>;
};
