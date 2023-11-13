import {t}    from "@use-pico/i18n";
import {Page} from "@use-pico/ui";

export default function Index() {
    return <Page
        text={{
            header: t()`Low-level system administration`,
        }}
    >
    </Page>;
};
