import {GameIcon} from "@derivean/ui";
import {
    t,
    tx
}                 from "@use-pico/i18n";
import {Page}     from "@use-pico/ui";

export default function Index() {
    return <Page
        icon={<GameIcon/>}
        text={{
            title:  tx()`DeRivean - The Invisible Game (title)`,
            header: t()`DeRivean - The Invisible Game`,
        }}
    >
    </Page>;
}
