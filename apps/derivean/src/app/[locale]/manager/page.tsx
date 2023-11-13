import {IconSubtask} from "@tabler/icons-react";
import {t}           from "@use-pico/i18n";
import {Page}        from "@use-pico/ui";

export default function Index() {
    return <Page
        icon={<IconSubtask/>}
        text={{
            header: t()`Game management`,
        }}
    >
    </Page>;
}
