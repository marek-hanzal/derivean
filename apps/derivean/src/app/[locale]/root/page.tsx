import {
    t,
    tx
}             from "@use-pico/i18n";
import {Page} from "@use-pico/ui";

export default function Index() {
    return <Page
        text={{
            title:  tx()`Low-level system administration`,
            header: t()`Low-level system administration`,
        }}
    >
    </Page>;
};
