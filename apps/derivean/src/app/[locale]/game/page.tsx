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
            title:  tx()`Game overview`,
            header: t()`Game overview`,
        }}
    >
        <h1>[Kingsomds]</h1>
    </Page>;
}
