import {tx}   from "@use-pico/i18n";
import {Page} from "@use-pico/ui";

export default function Index() {
    return <Page
        text={{
            title:  tx()`Low-level system administration`,
            header: tx()`Low-level system administration`,
        }}
        withActive={["/root"]}
    >
    </Page>;
};
