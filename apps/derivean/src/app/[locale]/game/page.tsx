import {GameIcon} from "@derivean/ui";
import {tx}       from "@use-pico/i18n";
import {Page}     from "@use-pico/ui";

export default function Index() {
    return <Page
        icon={<GameIcon/>}
        text={{
            title:  tx()`Game overview`,
            header: tx()`Game overview`,
        }}
    >
    </Page>;
}
