import {
    Icon,
    Page
}          from "@use-pico/client";
import {t} from "@use-pico/translator";

export default function Index() {
    return <Page
        icon={<Icon icon={"i-tabler:subtask"}/>}
        text={{
            header: t`Game management`,
        }}
    >
    </Page>;
}
