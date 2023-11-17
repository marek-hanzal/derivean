import {IconSubtask} from "@tabler/icons-react";
import {Page}        from "@use-pico/client";
import {t}           from "@use-pico/translator";

export default function Index() {
    return <Page
        icon={<IconSubtask/>}
        text={{
            header: t()`Game management`,
        }}
    >
    </Page>;
}
