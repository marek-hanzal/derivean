import {GameIcon} from "@derivean/ui";
import {t}        from "@use-pico/i18n";
import {Page}     from "@use-pico/ui";

export default function Index() {
    return <Page
        icon={<GameIcon/>}
        text={{
            header: t()`DeRivean - The Invisible Game`,
        }}
    >
    </Page>;
}
