import {GameIcon} from "@derivean/ui";
import {Page}     from "@use-pico/client";
import {t}        from "@use-pico/translator";

export default function Index() {
    return <Page
        icon={<GameIcon/>}
        text={{
            header: t`DeRivean - The Invisible Game`,
        }}
    >
    </Page>;
}
