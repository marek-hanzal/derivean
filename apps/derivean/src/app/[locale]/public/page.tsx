import {GameIcon} from "@derivean/ui";
import {tx}       from "@use-pico/i18n";
import {Page}     from "@use-pico/ui";

export default function Index() {
    return <Page
        icon={<GameIcon/>}
        text={{
            title:  tx()`DeRivean - The Invisible Game`,
            header: tx()`DeRivean - The Invisible Game`,
        }}
    >
    </Page>;
}
